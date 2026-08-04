# CowData Performance A/B (commit `5312fbf1688068ef89c44bfa0120a94966b4b11d`)

Date: 2026-03-26 (Australia/Melbourne)

## Environment
- Host: macOS arm64
- Build command:
  - `scons target=editor dev_build=yes compiledb=yes platform=macos arch=arm64 vulkan_sdk_path=/Users/ben/Development/VulkanSDK/1.4.304.1 generate_bundle=yes tests=yes -j8`
- Benchmark command (run 3 times per variant):
  - `./bin/godot.macos.editor.dev.arm64 --test cowdata-perf --cowdata-perf-runs=5 --cowdata-perf-warmup=1 --cowdata-perf-single-iters=800000 --cowdata-perf-multi-read-iters=2000000 --force-colors`

## Benchmark Scenarios
- `single_thread_copy_read`: copy `Vector<String>` + one indexed read per iteration.
- `multi_thread_shared_read`: create one shared `Vector<String>` once, then multiple threads perform read-only indexed access.

## Summary (15 measured runs per scenario per variant)
| Scenario | `5312...` avg us | parent avg us | Delta |
|---|---:|---:|---:|
| `single_thread_copy_read` | 72655.20 | 37791.67 | +92.25% |
| `multi_thread_shared_read` | 69545.87 | 51531.53 | +34.96% |

Detailed aggregates:
- `commit_5312|single_thread_copy_read`: count=15 avg_us=72655.20 min_us=66968 max_us=95304 avg_ns=90.82
- `commit_parent|single_thread_copy_read`: count=15 avg_us=37791.67 min_us=36842 max_us=39031 avg_ns=47.24
- `commit_5312|multi_thread_shared_read`: count=15 avg_us=69545.87 min_us=49816 max_us=151113 avg_ns=3.48
- `commit_parent|multi_thread_shared_read`: count=15 avg_us=51531.53 min_us=46821 max_us=56998 avg_ns=2.58

## Raw Output (`5312...`)
```text
=== commit_5312 pass=1 ===
COWDATA_PERF config runs=5 warmup=1 data_size=4096 single_iters=800000 multi_read_iters=2000000 multi_threads=10 worker_threads=10
COWDATA_PERF run=1 scenario=single_thread_copy_read threads=1 ops=800000 us=68344 ns_per_op=85.43 ops_per_sec=11705489.87 checksum=7783154
COWDATA_PERF run=1 scenario=multi_thread_shared_read threads=10 ops=20000000 us=60646 ns_per_op=3.03 ops_per_sec=329782673.22 checksum=194579957
COWDATA_PERF run=2 scenario=single_thread_copy_read threads=1 ops=800000 us=69396 ns_per_op=86.75 ops_per_sec=11528041.96 checksum=7783154
COWDATA_PERF run=2 scenario=multi_thread_shared_read threads=10 ops=20000000 us=60681 ns_per_op=3.03 ops_per_sec=329592458.92 checksum=194579957
COWDATA_PERF run=3 scenario=single_thread_copy_read threads=1 ops=800000 us=68379 ns_per_op=85.47 ops_per_sec=11699498.38 checksum=7783154
COWDATA_PERF run=3 scenario=multi_thread_shared_read threads=10 ops=20000000 us=51814 ns_per_op=2.59 ops_per_sec=385996062.84 checksum=194579957
COWDATA_PERF run=4 scenario=single_thread_copy_read threads=1 ops=800000 us=67474 ns_per_op=84.34 ops_per_sec=11856418.77 checksum=7783154
COWDATA_PERF run=4 scenario=multi_thread_shared_read threads=10 ops=20000000 us=57894 ns_per_op=2.89 ops_per_sec=345458942.20 checksum=194579957
COWDATA_PERF run=5 scenario=single_thread_copy_read threads=1 ops=800000 us=67012 ns_per_op=83.77 ops_per_sec=11938160.33 checksum=7783154
COWDATA_PERF run=5 scenario=multi_thread_shared_read threads=10 ops=20000000 us=51030 ns_per_op=2.55 ops_per_sec=391926317.85 checksum=194579957
=== commit_5312 pass=2 ===
COWDATA_PERF config runs=5 warmup=1 data_size=4096 single_iters=800000 multi_read_iters=2000000 multi_threads=10 worker_threads=10
COWDATA_PERF run=1 scenario=single_thread_copy_read threads=1 ops=800000 us=69057 ns_per_op=86.32 ops_per_sec=11584632.98 checksum=7783154
COWDATA_PERF run=1 scenario=multi_thread_shared_read threads=10 ops=20000000 us=85171 ns_per_op=4.26 ops_per_sec=234821711.62 checksum=194579957
COWDATA_PERF run=2 scenario=single_thread_copy_read threads=1 ops=800000 us=73406 ns_per_op=91.76 ops_per_sec=10898291.69 checksum=7783154
COWDATA_PERF run=2 scenario=multi_thread_shared_read threads=10 ops=20000000 us=151113 ns_per_op=7.56 ops_per_sec=132351286.79 checksum=194579957
COWDATA_PERF run=3 scenario=single_thread_copy_read threads=1 ops=800000 us=87596 ns_per_op=109.50 ops_per_sec=9132837.12 checksum=7783154
COWDATA_PERF run=3 scenario=multi_thread_shared_read threads=10 ops=20000000 us=99073 ns_per_op=4.95 ops_per_sec=201871347.39 checksum=194579957
COWDATA_PERF run=4 scenario=single_thread_copy_read threads=1 ops=800000 us=83256 ns_per_op=104.07 ops_per_sec=9608917.08 checksum=7783154
COWDATA_PERF run=4 scenario=multi_thread_shared_read threads=10 ops=20000000 us=75083 ns_per_op=3.75 ops_per_sec=266371881.78 checksum=194579957
COWDATA_PERF run=5 scenario=single_thread_copy_read threads=1 ops=800000 us=95304 ns_per_op=119.13 ops_per_sec=8394191.22 checksum=7783154
COWDATA_PERF run=5 scenario=multi_thread_shared_read threads=10 ops=20000000 us=82984 ns_per_op=4.15 ops_per_sec=241010315.24 checksum=194579957
=== commit_5312 pass=3 ===
COWDATA_PERF config runs=5 warmup=1 data_size=4096 single_iters=800000 multi_read_iters=2000000 multi_threads=10 worker_threads=10
COWDATA_PERF run=1 scenario=single_thread_copy_read threads=1 ops=800000 us=69100 ns_per_op=86.38 ops_per_sec=11577424.02 checksum=7783154
COWDATA_PERF run=1 scenario=multi_thread_shared_read threads=10 ops=20000000 us=51039 ns_per_op=2.55 ops_per_sec=391857207.23 checksum=194579957
COWDATA_PERF run=2 scenario=single_thread_copy_read threads=1 ops=800000 us=68069 ns_per_op=85.09 ops_per_sec=11752780.27 checksum=7783154
COWDATA_PERF run=2 scenario=multi_thread_shared_read threads=10 ops=20000000 us=59864 ns_per_op=2.99 ops_per_sec=334090605.37 checksum=194579957
COWDATA_PERF run=3 scenario=single_thread_copy_read threads=1 ops=800000 us=67518 ns_per_op=84.40 ops_per_sec=11848692.20 checksum=7783154
COWDATA_PERF run=3 scenario=multi_thread_shared_read threads=10 ops=20000000 us=56772 ns_per_op=2.84 ops_per_sec=352286338.34 checksum=194579957
COWDATA_PERF run=4 scenario=single_thread_copy_read threads=1 ops=800000 us=68949 ns_per_op=86.19 ops_per_sec=11602778.87 checksum=7783154
COWDATA_PERF run=4 scenario=multi_thread_shared_read threads=10 ops=20000000 us=49816 ns_per_op=2.49 ops_per_sec=401477436.97 checksum=194579957
COWDATA_PERF run=5 scenario=single_thread_copy_read threads=1 ops=800000 us=66968 ns_per_op=83.71 ops_per_sec=11946004.06 checksum=7783154
COWDATA_PERF run=5 scenario=multi_thread_shared_read threads=10 ops=20000000 us=50208 ns_per_op=2.51 ops_per_sec=398342893.56 checksum=194579957
```

## Raw Output (parent)
```text
=== commit_parent pass=1 ===
COWDATA_PERF config runs=5 warmup=1 data_size=4096 single_iters=800000 multi_read_iters=2000000 multi_threads=10 worker_threads=10
COWDATA_PERF run=1 scenario=single_thread_copy_read threads=1 ops=800000 us=37246 ns_per_op=46.56 ops_per_sec=21478816.52 checksum=7783154
COWDATA_PERF run=1 scenario=multi_thread_shared_read threads=10 ops=20000000 us=48459 ns_per_op=2.42 ops_per_sec=412720031.37 checksum=194579957
COWDATA_PERF run=2 scenario=single_thread_copy_read threads=1 ops=800000 us=37275 ns_per_op=46.59 ops_per_sec=21462105.97 checksum=7783154
COWDATA_PERF run=2 scenario=multi_thread_shared_read threads=10 ops=20000000 us=50606 ns_per_op=2.53 ops_per_sec=395210054.14 checksum=194579957
COWDATA_PERF run=3 scenario=single_thread_copy_read threads=1 ops=800000 us=37538 ns_per_op=46.92 ops_per_sec=21311737.44 checksum=7783154
COWDATA_PERF run=3 scenario=multi_thread_shared_read threads=10 ops=20000000 us=52216 ns_per_op=2.61 ops_per_sec=383024360.35 checksum=194579957
COWDATA_PERF run=4 scenario=single_thread_copy_read threads=1 ops=800000 us=38591 ns_per_op=48.24 ops_per_sec=20730222.07 checksum=7783154
COWDATA_PERF run=4 scenario=multi_thread_shared_read threads=10 ops=20000000 us=53227 ns_per_op=2.66 ops_per_sec=375749149.87 checksum=194579957
COWDATA_PERF run=5 scenario=single_thread_copy_read threads=1 ops=800000 us=39031 ns_per_op=48.79 ops_per_sec=20496528.40 checksum=7783154
COWDATA_PERF run=5 scenario=multi_thread_shared_read threads=10 ops=20000000 us=50107 ns_per_op=2.51 ops_per_sec=399145827.93 checksum=194579957
=== commit_parent pass=2 ===
COWDATA_PERF config runs=5 warmup=1 data_size=4096 single_iters=800000 multi_read_iters=2000000 multi_threads=10 worker_threads=10
COWDATA_PERF run=1 scenario=single_thread_copy_read threads=1 ops=800000 us=38381 ns_per_op=47.98 ops_per_sec=20843646.60 checksum=7783154
COWDATA_PERF run=1 scenario=multi_thread_shared_read threads=10 ops=20000000 us=49710 ns_per_op=2.49 ops_per_sec=402333534.50 checksum=194579957
COWDATA_PERF run=2 scenario=single_thread_copy_read threads=1 ops=800000 us=37235 ns_per_op=46.54 ops_per_sec=21485161.81 checksum=7783154
COWDATA_PERF run=2 scenario=multi_thread_shared_read threads=10 ops=20000000 us=50735 ns_per_op=2.54 ops_per_sec=394205183.80 checksum=194579957
COWDATA_PERF run=3 scenario=single_thread_copy_read threads=1 ops=800000 us=38366 ns_per_op=47.96 ops_per_sec=20851795.86 checksum=7783154
COWDATA_PERF run=3 scenario=multi_thread_shared_read threads=10 ops=20000000 us=46821 ns_per_op=2.34 ops_per_sec=427158753.55 checksum=194579957
COWDATA_PERF run=4 scenario=single_thread_copy_read threads=1 ops=800000 us=37613 ns_per_op=47.02 ops_per_sec=21269242.02 checksum=7783154
COWDATA_PERF run=4 scenario=multi_thread_shared_read threads=10 ops=20000000 us=52733 ns_per_op=2.64 ops_per_sec=379269148.35 checksum=194579957
COWDATA_PERF run=5 scenario=single_thread_copy_read threads=1 ops=800000 us=37457 ns_per_op=46.82 ops_per_sec=21357823.64 checksum=7783154
COWDATA_PERF run=5 scenario=multi_thread_shared_read threads=10 ops=20000000 us=52340 ns_per_op=2.62 ops_per_sec=382116927.78 checksum=194579957
=== commit_parent pass=3 ===
COWDATA_PERF config runs=5 warmup=1 data_size=4096 single_iters=800000 multi_read_iters=2000000 multi_threads=10 worker_threads=10
COWDATA_PERF run=1 scenario=single_thread_copy_read threads=1 ops=800000 us=36842 ns_per_op=46.05 ops_per_sec=21714347.76 checksum=7783154
COWDATA_PERF run=1 scenario=multi_thread_shared_read threads=10 ops=20000000 us=51944 ns_per_op=2.60 ops_per_sec=385030032.34 checksum=194579957
COWDATA_PERF run=2 scenario=single_thread_copy_read threads=1 ops=800000 us=38609 ns_per_op=48.26 ops_per_sec=20720557.38 checksum=7783154
COWDATA_PERF run=2 scenario=multi_thread_shared_read threads=10 ops=20000000 us=53700 ns_per_op=2.69 ops_per_sec=372439478.58 checksum=194579957
COWDATA_PERF run=3 scenario=single_thread_copy_read threads=1 ops=800000 us=38142 ns_per_op=47.68 ops_per_sec=20974254.10 checksum=7783154
COWDATA_PERF run=3 scenario=multi_thread_shared_read threads=10 ops=20000000 us=54237 ns_per_op=2.71 ops_per_sec=368751958.99 checksum=194579957
COWDATA_PERF run=4 scenario=single_thread_copy_read threads=1 ops=800000 us=37670 ns_per_op=47.09 ops_per_sec=21237058.67 checksum=7783154
COWDATA_PERF run=4 scenario=multi_thread_shared_read threads=10 ops=20000000 us=56998 ns_per_op=2.85 ops_per_sec=350889504.89 checksum=194579957
COWDATA_PERF run=5 scenario=single_thread_copy_read threads=1 ops=800000 us=36879 ns_per_op=46.10 ops_per_sec=21692562.16 checksum=7783154
COWDATA_PERF run=5 scenario=multi_thread_shared_read threads=10 ops=20000000 us=49140 ns_per_op=2.46 ops_per_sec=407000407.00 checksum=194579957
```

## Toggle Read/Write Race Test (without `5312...` fix)

New test command in `test_cowdata_perf.cpp`:
- `cowdata-toggle-race`
- Modes:
  - `--cowdata-toggle-mode=no_mutex`
  - `--cowdata-toggle-mode=mutex`

Command used (no mutex):
- `./bin/godot.macos.editor.dev.arm64 --test cowdata-toggle-race --cowdata-toggle-mode=no_mutex --cowdata-toggle-ms=3000 --cowdata-toggle-size=1024 --force-colors`

Observed:
- Crashes reliably on parent `cowdata.h`:
  - `ERROR: FATAL: Index p_index = 0 is out of bounds (size() = 0).`
  - backtrace top: `CowData<int>::get(long long) const` -> `Vector<int>::operator[]` -> `TestCowDataPerf::_run_toggle_read_case`

Command used (mutex):
- `./bin/godot.macos.editor.dev.arm64 --test cowdata-toggle-race --cowdata-toggle-mode=mutex --cowdata-toggle-ms=3000 --cowdata-toggle-size=1024 --force-colors`

Observed:
- Completed successfully:
  - `COWDATA_TOGGLE_RACE scenario=mutex_guarded mutex=1 duration_ms=3000 data_size=1024 reads=8933295 writes=23696944 invalid_reads=0`

## Timing: `mutex` (without fix) vs `no_mutex` (with fix)

Workload:
- `--cowdata-toggle-ms=5000 --cowdata-toggle-size=1024`
- 5 runs each.

Commands:
- Without fix (parent `cowdata.h`):
  - `./bin/godot.macos.editor.dev.arm64 --test cowdata-toggle-race --cowdata-toggle-mode=mutex --cowdata-toggle-ms=5000 --cowdata-toggle-size=1024 --force-colors`
- With fix (`5312...` `cowdata.h`):
  - `./bin/godot.macos.editor.dev.arm64 --test cowdata-toggle-race --cowdata-toggle-mode=no_mutex --cowdata-toggle-ms=5000 --cowdata-toggle-size=1024 --force-colors`

Averages over 5 runs:
- `mutex` (without fix):
  - `reads/s = 3,133,247.0`
  - `writes/s = 7,821,699.64`
  - `invalid_reads = 0`
- `no_mutex` (with fix):
  - `reads/s = 16,733,572.4`
  - `writes/s = 9,543,402.6`
  - `invalid_reads = 2,046,321.4` per run average
  - `invalid_rate = 2.446%` (`invalid_reads / reads`)

Relative throughput (`no_mutex with fix` vs `mutex without fix`):
- Read throughput: `5.34x`
- Write throughput: `1.22x`

## Volatile Pointer A/B (`5312...` with vs without `volatile`)

Goal:
- Compare the same `5312...` logic with only one source change:
  - `mutable T *volatile _ptr = nullptr;` (volatile ON)
  - `mutable T *_ptr = nullptr;` (volatile OFF)

Build command:
- `scons target=editor dev_build=yes compiledb=yes platform=macos arch=arm64 vulkan_sdk_path=/Users/ben/Development/VulkanSDK/1.4.304.1 generate_bundle=yes tests=yes -j8`

Run commands:
- `for i in 1 2 3; do ./bin/godot.macos.editor.dev.arm64 --test cowdata-perf --cowdata-perf-runs=5 --cowdata-perf-warmup=1 --cowdata-perf-single-iters=800000 --cowdata-perf-multi-read-iters=2000000 --force-colors; done`
- `for i in 1 2 3 4 5; do ./bin/godot.macos.editor.dev.arm64 --test cowdata-toggle-race --cowdata-toggle-mode=no_mutex --cowdata-toggle-ms=5000 --cowdata-toggle-size=1024 --force-colors; done`

Aggregated `cowdata-perf` results (15 measured samples/scenario):

| Scenario | Volatile ON median us | Volatile OFF median us | Delta (OFF vs ON) | Volatile ON mean us | Volatile OFF mean us |
|---|---:|---:|---:|---:|---:|
| `single_thread_copy_read` | 68,041 | 66,672 | -2.01% | 68,282.47 | 70,869.07 |
| `multi_thread_shared_read` | 48,213 | 56,249 | +16.67% | 52,246.13 | 57,472.07 |

Notes:
- Volatile OFF `single_thread_copy_read` includes one outlier at `121,364 us`, which inflates the mean; median is more stable here.
- For read-heavy multithread scenario in this run set, volatile OFF is slower.

Aggregated `cowdata-toggle-race` (`no_mutex`, 5s, 5 runs):

| Metric | Volatile ON median | Volatile OFF median | Delta (OFF vs ON) |
|---|---:|---:|---:|
| `reads` | 80,942,354 | 80,158,946 | -0.97% |
| `writes` | 47,920,065 | 46,284,020 | -3.41% |
| `invalid_reads` | 2,300,977 | 2,010,643 | -12.62% |

Raw logs for this A/B run:
- `/tmp/volatile_on_cowdata_perf.txt`
- `/tmp/volatile_off_cowdata_perf.txt`
- `/tmp/volatile_on_toggle.txt`
- `/tmp/volatile_off_toggle.txt`

## Extended Single-Thread A/B (Fix vs Parent Baseline)

Purpose:
- Compare the no-`volatile` fix variant against true pre-commit baseline (parent of `5312fbf1688068ef89c44bfa0120a94966b4b11d`).

Build command (each variant):
- `scons target=editor dev_build=yes compiledb=yes platform=macos arch=arm64 vulkan_sdk_path=/Users/ben/Development/VulkanSDK/1.4.304.1 generate_bundle=yes tests=yes -j8`

Benchmark command (run 4 passes per variant):
- `./bin/godot.macos.editor.dev.arm64 --test cowdata-perf --cowdata-perf-runs=40 --cowdata-perf-warmup=5 --cowdata-perf-single-iters=5000000 --cowdata-perf-multi-read-iters=1 --cowdata-perf-multi-threads=1 --force-colors`

Collected samples:
- `single_thread_copy_read`: 160 samples per variant (`4 passes * 40 measured runs`).

Aggregate results (`single_thread_copy_read`):

| Metric | Fix (no-volatile) | Parent baseline (no fix) | Delta |
|---|---:|---:|---:|
| `median us` | 423,159 | 234,352 | +80.57% |
| `mean us` | 420,986.59 | 233,450.38 | +80.33% |
| `p05 us` | 406,610.00 | 224,900.35 | +80.80% |
| `p95 us` | 427,999.25 | 237,417.00 | +80.27% |
| `median ns/op` | 84.63 | 46.87 | +80.56% |

Per-pass medians (sanity check):
- Fix: `422,495`, `421,438`, `425,037`, `423,901` us.
- Parent: `233,155.5`, `233,635.5`, `235,403.5`, `235,702` us.

Raw logs:
- `/tmp/single_focus_fix_no_volatile.txt`
- `/tmp/single_focus_parent_no_fix.txt`

## Extended Single-Thread A/B/C (Baseline vs Old Hash vs Cheap Hash)

Change tested:
- In `CowData::get_reader_count()`, replaced Murmur-style two-multiply hash mixing with a cheaper xor-shift mix:
  - old: 2 multiplies + 3 xor-shift rounds
  - new: `hash ^= hash >> 7; hash ^= hash >> 13;`

Command:
- `./bin/godot.macos.editor.dev.arm64 --test cowdata-perf --cowdata-perf-runs=40 --cowdata-perf-warmup=5 --cowdata-perf-single-iters=5000000 --cowdata-perf-multi-read-iters=1 --cowdata-perf-multi-threads=1 --force-colors`
- 4 passes per variant, 160 `single_thread_copy_read` samples/variant.

Aggregate (`single_thread_copy_read`):

| Variant | Median us | Mean us | p05 us | p95 us | Median ns/op |
|---|---:|---:|---:|---:|---:|
| Baseline (no fix) | 234,352.0 | 233,450.38 | 224,900.35 | 237,417.0 | 46.87 |
| Fix + old hash | 423,159.0 | 420,986.59 | 406,610.0 | 427,999.25 | 84.63 |
| Fix + cheap hash | 365,328.5 | 364,598.32 | 350,011.7 | 369,512.75 | 73.07 |

Relative deltas:
- Cheap hash vs old hash:
  - median: `-13.67%`
  - mean: `-13.39%`
  - p05: `-13.92%`
  - p95: `-13.67%`
- Cheap hash vs baseline:
  - median: `+55.89%`
  - mean: `+56.18%`
  - p05: `+55.63%`
  - p95: `+55.64%`

Notes:
- One outlier (`506,290 us`) occurred in cheap-hash pass 3; median and percentile comparisons remain stable.

Raw log:
- `/tmp/single_focus_fix_cheaphash.txt`

## Extended Single-Thread A/B/C/D (Add Plain Increment Under Reader Guard)

Change tested on top of cheap hash:
- In `CowData::_ref`, for non-null `from_ptr`, use:
  - `_get_refcount((uint8_t *)from_ptr)->increment();`
  - instead of `conditional_increment()`.

Rationale:
- Reader bucket is incremented before reading `p_from._ptr`, and writer side waits for reader bucket to drop before final unref/free.
- Under the intended single-writer model, this means refcount reaching zero before this increment should not happen.

Aggregate (`single_thread_copy_read`, 160 samples):

| Variant | Median us | Mean us | p05 us | p95 us | Median ns/op |
|---|---:|---:|---:|---:|---:|
| Baseline (no fix) | 234,352.0 | 233,450.38 | 224,900.35 | 237,417.0 | 46.87 |
| Fix + old hash | 423,159.0 | 420,986.59 | 406,610.0 | 427,999.25 | 84.63 |
| Fix + cheap hash | 365,328.5 | 364,598.32 | 350,011.7 | 369,512.75 | 73.07 |
| Fix + cheap hash + plain increment | 299,790.0 | 306,054.72 | 287,839.6 | 350,973.9 | 59.96 |

Relative deltas for `Fix + cheap hash + plain increment`:
- vs cheap hash:
  - median: `-17.94%`
  - mean: `-16.06%`
  - p05: `-17.76%`
  - p95: `-5.02%`
- vs baseline:
  - median: `+27.92%`
  - mean: `+31.10%`
  - p05: `+27.99%`
  - p95: `+47.83%`

Stability note:
- Pass 1 had several large outliers (e.g. `557,248 us`), while passes 2-4 were tightly clustered near `~299k us` median.

Quick safety check:
- `cowdata-toggle-race --cowdata-toggle-mode=no_mutex --cowdata-toggle-ms=20000 --cowdata-toggle-size=1024`
- Completed (no crash): `reads=237433437 writes=189772089 invalid_reads=16088622`

Raw log:
- `/tmp/single_focus_fix_cheaphash_plaininc.txt`
