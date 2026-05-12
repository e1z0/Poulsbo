#!/usr/bin/env bash
set -euo pipefail

target_host="${TARGET_HOST:-10.1.11.218}"
target_user="${TARGET_USER:-root}"
timeout_secs="${TIMEOUT_SECS:-15}"
probe_file="${PROBE_FILE:-/etc/hostname}"
target_probe_path="${TARGET_PROBE_PATH:-/tmp/codex-target-health.hostname}"

tmpdir="$(mktemp -d)"
cleanup() {
    rm -rf "$tmpdir"
}
trap cleanup EXIT

ping_status="unknown"
tcp_status="unknown"
ssh_none_status="unknown"
ssh_pubkey_status="unknown"
scp_status="unknown"
ssh_none_kex="0"
ssh_pubkey_kex="0"

run_capture() {
    local name="$1"
    shift
    local status_file="$tmpdir/${name}.status"
    local log_file="$tmpdir/${name}.log"

    (
        set +e
        "$@" >"$log_file" 2>&1
        echo $? >"$status_file"
    )
}

classify_ssh_log() {
    local log_file="$1"
    local rc="$2"
    local mode="$3"
    local kex="0"
    local status="unknown"

    if grep -q "SSH2_MSG_NEWKEYS received" "$log_file" 2>/dev/null; then
        kex="1"
    fi

    if [[ "$rc" == "0" ]]; then
        status="ok"
    elif [[ "$rc" == "124" ]]; then
        if [[ "$kex" == "1" ]]; then
            status="timeout_post_kex"
        else
            status="timeout_pre_auth"
        fi
    else
        if grep -qi "permission denied" "$log_file" 2>/dev/null; then
            status="auth_rejected"
        elif grep -qi "connection refused" "$log_file" 2>/dev/null; then
            status="connection_refused"
        elif grep -qi "no route to host" "$log_file" 2>/dev/null; then
            status="no_route"
        elif grep -qi "connection timed out" "$log_file" 2>/dev/null; then
            status="connect_timeout"
        else
            status="error_${rc}"
        fi
    fi

    if [[ "$mode" == "none" ]]; then
        ssh_none_kex="$kex"
        ssh_none_status="$status"
    else
        ssh_pubkey_kex="$kex"
        ssh_pubkey_status="$status"
    fi
}

if ping -c 1 -W 1 "$target_host" >"$tmpdir/ping.log" 2>&1; then
    ping_status="ok"
else
    ping_status="fail"
fi

if nc -zvw2 "$target_host" 22 >"$tmpdir/tcp.log" 2>&1; then
    tcp_status="open"
else
    tcp_status="closed"
fi

run_capture ssh_none \
    timeout "$timeout_secs" ssh -vvv \
        -o BatchMode=yes \
        -o ConnectTimeout=5 \
        -o PreferredAuthentications=none \
        -o PubkeyAuthentication=no \
        -o PasswordAuthentication=no \
        -o KbdInteractiveAuthentication=no \
        -o GSSAPIAuthentication=no \
        -o NumberOfPasswordPrompts=0 \
        "${target_user}@${target_host}" "true"

classify_ssh_log "$tmpdir/ssh_none.log" "$(cat "$tmpdir/ssh_none.status")" "none"

run_capture ssh_pubkey \
    timeout "$timeout_secs" ssh -vvv \
        -o BatchMode=yes \
        -o ConnectTimeout=5 \
        -o PreferredAuthentications=publickey \
        -o PubkeyAuthentication=yes \
        -o PasswordAuthentication=no \
        -o KbdInteractiveAuthentication=no \
        -o GSSAPIAuthentication=no \
        -o NumberOfPasswordPrompts=0 \
        "${target_user}@${target_host}" "echo ok"

classify_ssh_log "$tmpdir/ssh_pubkey.log" "$(cat "$tmpdir/ssh_pubkey.status")" "pubkey"

run_capture scp_probe \
    timeout "$timeout_secs" scp -v \
        -o BatchMode=yes \
        -o ConnectTimeout=5 \
        "$probe_file" "${target_user}@${target_host}:${target_probe_path}"

scp_rc="$(cat "$tmpdir/scp_probe.status")"
if [[ "$scp_rc" == "0" ]]; then
    scp_status="ok"
elif [[ "$scp_rc" == "124" ]]; then
    if grep -q "SSH2_MSG_NEWKEYS received" "$tmpdir/scp_probe.log" 2>/dev/null; then
        scp_status="timeout_post_kex"
    else
        scp_status="timeout_pre_auth"
    fi
else
    scp_status="error_${scp_rc}"
fi

overall="unknown"
if [[ "$ssh_pubkey_status" == "ok" ]]; then
    overall="remote_usable"
elif [[ "$tcp_status" != "open" ]]; then
    overall="ssh_port_down"
elif [[ "$ssh_none_status" == "timeout_post_kex" &&
        "$ssh_pubkey_status" == "timeout_post_kex" ]]; then
    overall="ssh_stalled_post_kex"
elif [[ "$ping_status" != "ok" ]]; then
    overall="host_down"
elif [[ "$ssh_pubkey_status" != "ok" ]]; then
    overall="ssh_not_usable"
fi

echo "target_host=${target_host}"
echo "target_user=${target_user}"
echo "timeout_secs=${timeout_secs}"
echo "ping_status=${ping_status}"
echo "tcp_22_status=${tcp_status}"
echo "ssh_none_status=${ssh_none_status}"
echo "ssh_none_kex=${ssh_none_kex}"
echo "ssh_pubkey_status=${ssh_pubkey_status}"
echo "ssh_pubkey_kex=${ssh_pubkey_kex}"
echo "scp_status=${scp_status}"
echo "overall=${overall}"

if [[ "${SHOW_LOGS:-0}" == "1" ]]; then
    echo "PING_LOG"
    cat "$tmpdir/ping.log"
    echo "TCP_LOG"
    cat "$tmpdir/tcp.log"
    echo "SSH_NONE_LOG"
    cat "$tmpdir/ssh_none.log"
    echo "SSH_PUBKEY_LOG"
    cat "$tmpdir/ssh_pubkey.log"
    echo "SCP_LOG"
    cat "$tmpdir/scp_probe.log"
fi
