# SGX Legacy Mode Gate Validation

## What Changed

Validated the explicit legacy escape hatches that were added to the focused and
multi-submit wrapper layers:

- `SGX_OBJECT_DRIVER_MODE=legacy`
- `SGX_SUBMIT_DRIVER_MODE=legacy`
- `SGX_MULTI_SUBMIT_DRIVER_MODE=legacy`

During validation, the focused legacy submit path exposed a real bug in the old
sysfs-heavy script:

- `scripts/run-vaio-sgx-submit.sh`

Its fallback range scan only searched up to `max_before_range_id + 128`, which
was no longer enough once live GEM range IDs on the VAIO had advanced further.

That path now has an explicit scan bound:

- `RANGE_SCAN_MAX`

with current default:

- `512`

and the legacy script now errors cleanly if `RANGE_SCAN_MAX < 1`.

The same symmetry fix was then applied to the direct legacy multi-submit path:

- `scripts/run-vaio-sgx-multi-submit.sh`

which had still been hard-coded to search only up to
`max_before_range_id + 256`.

## Hardware Validation

Validated on May 11, 2026 on `10.1.11.218` with:

- focused object legacy wrapper:
  - `logs/vaio-legacy/20260511-130407-sgx-object-wrapper-legacy-live`
- focused submit legacy wrapper after the scan-range fix:
  - `logs/vaio-legacy/20260511-130601-sgx-submit-wrapper-legacy-live-rerun`
- indexed multi-submit legacy wrapper:
  - `logs/vaio-legacy/20260511-130640-sgx-indexed-wrapper-legacy-live`
- indexed multi-submit legacy wrapper after the same range-scan symmetry fix:
  - `logs/vaio-legacy/20260511-131035-sgx-indexed-wrapper-legacy-live-rerun`

Final target state after all three runs:

- `overall=remote_usable`
- `driver_srcversion=D43951B174F84BE870230AB`
- `override_module_size=385384`
- `rollback_pending_present=0`
- `rollback_backup_present=1`

### Focused Object Legacy

The explicit legacy object wrapper stayed on the old sysfs-heavy path:

- `REMOTE_BIN=/root/poulsbo-workloads/drm_probe`
- `OBJECT_NODE=/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_object`
- `OBJECTS_NODE=/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_objects`

The saved run still completed cleanly:

- `RANGE_ID=153`
- `OBJECT_ID=144`
- post-bind:
  - `GTT_STATE_AFTER_BIND`
  - `OBJECTS_AFTER_BIND`
- post-unbind:
  - `GTT_STATE_AFTER_UNBIND`
  - `OBJECTS_AFTER_UNBIND`
- post-drop:
  - `GTT_STATE_AFTER_DROP`
  - `OBJECTS_AFTER_DROP`

### Focused Submit Legacy

The first explicit legacy submit run failed before the fix with:

- `Expected 5 fresh SGX-adoptable GEM ranges, found 2`

because the sysfs fallback window was too narrow once the live range IDs had
advanced past `max_before_range_id + 128`.

After the `RANGE_SCAN_MAX` fix, the explicit legacy submit wrapper again stayed
on the old sysfs-heavy path:

- `REMOTE_BIN=/root/poulsbo-workloads/drm_probe`
- `QUEUE_MODE=wrap`
- `SUBMIT_NODE=/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submit`
- `SUBMITS_NODE=/sys/devices/pci0000:00/0000:00:02.0/poulsbo_sgx_submits`

The rerun completed cleanly:

- `RANGE_IDS=159 160 161 162 163`
- `OBJECT_IDS=147 148 149 150 151`
- `SUBMIT_ID=37`
- verified milestones in the saved log:
  - `SUBMITS_AFTER_VALIDATE`
  - `SUBMITS_AFTER_ISSUE`
  - `SUBMITS_AFTER_ISSUE_VALIDATE`
  - `SUBMITS_AFTER_DROP`
  - `OBJECTS_AFTER_DROP`

Final legacy submit cleanup returned to zero:

- `submits_shown=0 submits_total=0 truncated=0`
- `objects_shown=0 objects_total=0 truncated=0`

### Indexed Multi-Submit Legacy

The explicit legacy indexed wrapper stayed on the old multi-submit path:

- `REMOTE_BIN=/root/poulsbo-workloads/drm_probe`
- `DEPENDENCY_INTERFACE=indexed`

The saved run staged the expected four-submit legacy scenario:

- `SUBMIT_A_ID=38`
- `SUBMIT_B_ID=39`
- `SUBMIT_C_ID=40`
- `SUBMIT_D_ID=41`
- `RANGE_IDS=164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183`
- `OBJECT_IDS_A=152 153 154 155 156`
- `OBJECT_IDS_B=157 158 159 160 161`
- `OBJECT_IDS_C=162 163 164 165 166`
- `OBJECT_IDS_D=167 168 169 170 171`

The same legacy wrapper completed and cleaned up correctly:

- `SUBMITS_AFTER_DROP`
- `OBJECTS_AFTER_DROP`
- `submits_shown=0 submits_total=0 truncated=0`
- `objects_shown=0 objects_total=0 truncated=0`
- `live_submit_count=0`

After the multi-submit-side `RANGE_SCAN_MAX` symmetry fix, the explicit legacy
indexed wrapper was revalidated and still completed cleanly:

- `SUBMIT_A_ID=42`
- `SUBMIT_B_ID=43`
- `SUBMIT_C_ID=44`
- `SUBMIT_D_ID=45`
- `RANGE_IDS=184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200 201 202 203`
- `OBJECT_IDS_A=172 173 174 175 176`
- `OBJECT_IDS_B=177 178 179 180 181`
- `OBJECT_IDS_C=182 183 184 185 186`
- `OBJECT_IDS_D=187 188 189 190 191`
- final legacy cleanup again returned:
  - `SUBMITS_AFTER_DROP`
  - `OBJECTS_AFTER_DROP`
  - `COORD_STATE_AFTER_DROP`

## Conclusion

The explicit legacy modes are not just preserved in theory; they are usable on
the VAIO after current wrapper refactors.

The only required fix in this step was the focused legacy submit range-scan
window. After that, the same `RANGE_SCAN_MAX` symmetry fix was applied to the
legacy multi-submit path as well. With those fixes in place, the repo now has
validated behavior for:

- typed-by-default wrappers
- explicit legacy object path
- explicit legacy focused submit path
- explicit legacy indexed multi-submit path
