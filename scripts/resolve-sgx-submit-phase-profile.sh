#!/usr/bin/env bash
set -euo pipefail

phase_profile="${1:-${PHASE_PROFILE:-full}}"

usage() {
    cat <<'EOF'
Usage:
  resolve-sgx-submit-phase-profile.sh [profile]

Profiles:
  full
    Full focused submit-op phase ladder.
  post_descriptor
    Narrow post-descriptor isolation window for the current SGX `plan`
    investigation line.
  plan_query
    Split rerun for the current narrowed `plan_query` boundary.
  plan_query_fetch
    Isolate the first post-`plan` query fetch step only.
  plan_query_basic
    Isolate the basic post-`plan` query assertions window.
  plan_query_offset
    Isolate the post-`plan` offset assertion only.
  plan_query_config
    Isolate the post-`plan` config-count assertion only.
  plan_query_snapshot
    Isolate the full post-`plan` snapshot readback only.
  prepare_op
    Isolate the typed `prepare` submit-op only.
  prepare_query
    Isolate the post-`prepare` typed query assertions only.
  arm
    Isolate the typed `arm` submit-op only.
  emit
    Isolate the typed `emit` submit-op only.
  validate
    Isolate the typed `validate` submit-op only.
  launch
    Isolate the typed `launch` submit-op only.
  issue
    Isolate the typed `issue` submit-op only.
  running
    Isolate the running-state transition only.
  complete
    Isolate the completion-state transition only.
EOF
}

if [[ "${phase_profile}" == "-h" || "${phase_profile}" == "--help" ]]; then
    usage
    exit 0
fi

case "${phase_profile}" in
    full)
        printf '%s\n' 'sync,owner,report,manifest,descriptor_op,descriptor_query,plan_op,plan_query_fetch,plan_query_offset,plan_query_config,plan_query_snapshot,prepare_op,prepare_query,arm,emit,validate,launch,issue,running,complete'
        ;;
    post_descriptor)
        printf '%s\n' 'descriptor_query,plan_op,plan_query_fetch,plan_query_offset,plan_query_config,plan_query_snapshot,prepare_op,prepare_query'
        ;;
    plan_query)
        printf '%s\n' 'plan_query_fetch,plan_query_offset,plan_query_config,plan_query_snapshot'
        ;;
    plan_query_fetch)
        printf '%s\n' 'plan_query_fetch'
        ;;
    plan_query_basic)
        printf '%s\n' 'plan_query_offset,plan_query_config'
        ;;
    plan_query_offset)
        printf '%s\n' 'plan_query_offset'
        ;;
    plan_query_config)
        printf '%s\n' 'plan_query_config'
        ;;
    plan_query_snapshot)
        printf '%s\n' 'plan_query_snapshot'
        ;;
    prepare_op)
        printf '%s\n' 'prepare_op'
        ;;
    prepare_query)
        printf '%s\n' 'prepare_query'
        ;;
    arm)
        printf '%s\n' 'arm'
        ;;
    emit)
        printf '%s\n' 'emit'
        ;;
    validate)
        printf '%s\n' 'validate'
        ;;
    launch)
        printf '%s\n' 'launch'
        ;;
    issue)
        printf '%s\n' 'issue'
        ;;
    running)
        printf '%s\n' 'running'
        ;;
    complete)
        printf '%s\n' 'complete'
        ;;
    *)
        echo "Unknown SGX submit phase profile: ${phase_profile}" >&2
        echo "Valid profiles: full post_descriptor plan_query plan_query_fetch plan_query_basic plan_query_offset plan_query_config plan_query_snapshot prepare_op prepare_query arm emit validate launch issue running complete" >&2
        exit 1
        ;;
esac
