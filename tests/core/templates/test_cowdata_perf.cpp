/**************************************************************************/
/*  test_cowdata_perf.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_cowdata_perf)

#include "core/object/worker_thread_pool.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/string/print_string.h"
#include "core/templates/vector.h"

namespace TestCowDataPerf {

struct PerfConfig {
	int runs = 5;
	int warmup_runs = 1;
	int data_size = 4096;
	int single_iters = 400000;
	int multi_read_iters = 10000;
	int multi_threads = 0; // 0 means "use worker pool thread count".
};

static int _find_int_option(const List<String> &p_args, const String &p_prefix, int p_default_value) {
	for (const String &arg : p_args) {
		if (arg.begins_with(p_prefix)) {
			return arg.substr(p_prefix.length()).to_int();
		}
	}
	return p_default_value;
}

static PerfConfig _parse_config() {
	const List<String> args = OS::get_singleton()->get_cmdline_args();
	PerfConfig config;
	config.runs = MAX(1, _find_int_option(args, "--cowdata-perf-runs=", config.runs));
	config.warmup_runs = MAX(0, _find_int_option(args, "--cowdata-perf-warmup=", config.warmup_runs));
	config.data_size = MAX(2, _find_int_option(args, "--cowdata-perf-data-size=", config.data_size));
	config.single_iters = MAX(1, _find_int_option(args, "--cowdata-perf-single-iters=", config.single_iters));
	config.multi_read_iters = MAX(1, _find_int_option(args, "--cowdata-perf-multi-read-iters=", config.multi_read_iters));
	config.multi_threads = MAX(0, _find_int_option(args, "--cowdata-perf-multi-threads=", config.multi_threads));
	return config;
}

static Vector<String> _build_data(int p_size) {
	Vector<String> out;
	out.resize(p_size);
	for (int i = 0; i < p_size; i++) {
		out.set(i, "entry_" + String::num_int64(i));
	}
	return out;
}

static uint64_t _run_single_thread_copy_read(const Vector<String> &p_shared, int p_iters, uint64_t &r_checksum) {
	const int size = p_shared.size();
	r_checksum = 0;

	const uint64_t start = OS::get_singleton()->get_ticks_usec();
	for (int i = 0; i < p_iters; i++) {
		Vector<String> local = p_shared;
		r_checksum += local[(i * 17 + 3) % size].length();
	}
	return OS::get_singleton()->get_ticks_usec() - start;
}

struct MultiReadContext {
	const Vector<String> *shared = nullptr;
	int read_iters = 0;
	SafeNumeric<uint64_t> checksum;
};

static void _multi_read_task(void *p_userdata, uint32_t p_index) {
	MultiReadContext *ctx = static_cast<MultiReadContext *>(p_userdata);

	const int size = ctx->shared->size();
	uint64_t local_checksum = 0;
	for (int i = 0; i < ctx->read_iters; i++) {
		local_checksum += (*(ctx->shared))[(i * 31 + p_index) % size].length();
	}

	ctx->checksum.add(local_checksum);
}

static uint64_t _run_multi_thread_read(const Vector<String> &p_shared, int p_threads, int p_read_iters, uint64_t &r_checksum) {
	MultiReadContext ctx;
	ctx.shared = &p_shared;
	ctx.read_iters = p_read_iters;
	ctx.checksum.set(0);
	const uint64_t start = OS::get_singleton()->get_ticks_usec();

	WorkerThreadPool::GroupID group = WorkerThreadPool::get_singleton()->add_native_group_task(
			&_multi_read_task,
			&ctx,
			p_threads,
			p_threads,
			true,
			"cowdata-perf-read");
	WorkerThreadPool::get_singleton()->wait_for_group_task_completion(group);
	const uint64_t elapsed = OS::get_singleton()->get_ticks_usec() - start;

	r_checksum = ctx.checksum.get();
	return elapsed;
}

static void _print_result_line(
		int p_run,
		const String &p_scenario,
		uint64_t p_us,
		uint64_t p_ops,
		uint64_t p_checksum,
		int p_threads = 1) {
	const double ns_per_op = (double(p_us) * 1000.0) / double(p_ops);
	const double ops_per_sec = (double(p_ops) * 1000000.0) / double(p_us);
	print_line(vformat(
			"COWDATA_PERF run=%d scenario=%s threads=%d ops=%d us=%d ns_per_op=%.2f ops_per_sec=%.2f checksum=%d",
			p_run,
			p_scenario,
			p_threads,
			p_ops,
			p_us,
			ns_per_op,
			ops_per_sec,
			p_checksum));
}

static void run_cowdata_perf() {
	const PerfConfig config = _parse_config();
	const int worker_threads = MAX(1, WorkerThreadPool::get_singleton()->get_thread_count());
	const int multi_threads = config.multi_threads > 0 ? config.multi_threads : worker_threads;
	const Vector<String> shared = _build_data(config.data_size);

	print_line(vformat(
			"COWDATA_PERF config runs=%d warmup=%d data_size=%d single_iters=%d multi_read_iters=%d multi_threads=%d worker_threads=%d",
			config.runs,
			config.warmup_runs,
			config.data_size,
			config.single_iters,
			config.multi_read_iters,
			multi_threads,
			worker_threads));

	for (int i = 0; i < config.warmup_runs; i++) {
		uint64_t checksum = 0;
		_run_single_thread_copy_read(shared, config.single_iters, checksum);
		_run_multi_thread_read(shared, multi_threads, config.multi_read_iters, checksum);
	}

	for (int run = 1; run <= config.runs; run++) {
		uint64_t single_checksum = 0;
		const uint64_t single_us = _run_single_thread_copy_read(shared, config.single_iters, single_checksum);
		_print_result_line(run, "single_thread_copy_read", single_us, config.single_iters, single_checksum, 1);

		uint64_t multi_checksum = 0;
		const uint64_t multi_us = _run_multi_thread_read(shared, multi_threads, config.multi_read_iters, multi_checksum);
		const uint64_t total_multi_ops = uint64_t(multi_threads) * uint64_t(config.multi_read_iters);
		_print_result_line(run, "multi_thread_shared_read", multi_us, total_multi_ops, multi_checksum, multi_threads);
	}
}

REGISTER_TEST_COMMAND("cowdata-perf", &run_cowdata_perf);

struct ToggleRaceConfig {
	int duration_ms = 3000;
	int data_size = 1024;
	String mode = "both";
};

static ToggleRaceConfig _parse_toggle_race_config() {
	const List<String> args = OS::get_singleton()->get_cmdline_args();
	ToggleRaceConfig config;
	config.duration_ms = MAX(1, _find_int_option(args, "--cowdata-toggle-ms=", config.duration_ms));
	config.data_size = MAX(2, _find_int_option(args, "--cowdata-toggle-size=", config.data_size));
	for (const String &arg : args) {
		if (arg.begins_with("--cowdata-toggle-mode=")) {
			config.mode = arg.substr(String("--cowdata-toggle-mode=").length());
		}
	}
	return config;
}

struct ToggleRaceContext {
	Vector<int> shared;
	Vector<int> value_a;
	Vector<int> value_b;
	Mutex mutex;
	SafeFlag stop;
	SafeNumeric<uint64_t> writes;
	bool use_mutex = false;
};

static void _toggle_writer_thread(void *p_userdata) {
	ToggleRaceContext *ctx = static_cast<ToggleRaceContext *>(p_userdata);
	bool use_a = false;

	while (!ctx->stop.is_set()) {
		if (ctx->use_mutex) {
			MutexLock lock(ctx->mutex);
			ctx->shared = use_a ? ctx->value_a : ctx->value_b;
		} else {
			ctx->shared = use_a ? ctx->value_a : ctx->value_b;
		}
		use_a = !use_a;
		ctx->writes.increment();
	}
}

static void _run_toggle_read_case(const ToggleRaceConfig &p_config, bool p_use_mutex, const char *p_label) {
	ToggleRaceContext ctx;
	ctx.use_mutex = p_use_mutex;
	ctx.stop.clear();
	ctx.writes.set(0);

	ctx.value_a.resize(p_config.data_size);
	ctx.value_b.resize(p_config.data_size);
	for (int i = 0; i < p_config.data_size; i++) {
		ctx.value_a.set(i, 111);
		ctx.value_b.set(i, 222);
	}
	ctx.shared = ctx.value_a;

	Thread writer;
	writer.start(_toggle_writer_thread, &ctx);

	uint64_t reads = 0;
	uint64_t invalid_reads = 0;
	const uint64_t start = OS::get_singleton()->get_ticks_msec();

	while (OS::get_singleton()->get_ticks_msec() - start < uint64_t(p_config.duration_ms)) {
		int size = 0;
		int first = 0;
		int last = 0;
		if (p_use_mutex) {
			MutexLock lock(ctx.mutex);
			size = ctx.shared.size();
			first = ctx.shared[0];
			last = ctx.shared[size - 1];
		} else {
			size = ctx.shared.size();
			first = ctx.shared[0];
			last = ctx.shared[size - 1];
		}

		const bool valid_size = size == p_config.data_size;
		const bool valid_payload = (first == 111 && last == 111) || (first == 222 && last == 222);
		if (!valid_size || !valid_payload) {
			invalid_reads++;
		}
		reads++;
	}

	ctx.stop.set();
	writer.wait_to_finish();

	const uint64_t writes = ctx.writes.get();
	print_line(vformat(
			"COWDATA_TOGGLE_RACE scenario=%s mutex=%d duration_ms=%d data_size=%d reads=%d writes=%d invalid_reads=%d",
			p_label,
			p_use_mutex ? 1 : 0,
			p_config.duration_ms,
			p_config.data_size,
			reads,
			writes,
			invalid_reads));
}

static void run_cowdata_toggle_race() {
	const ToggleRaceConfig config = _parse_toggle_race_config();
	print_line(vformat("COWDATA_TOGGLE_RACE config duration_ms=%d data_size=%d mode=%s", config.duration_ms, config.data_size, config.mode));

	if (config.mode == "no_mutex" || config.mode == "both") {
		_run_toggle_read_case(config, false, "no_mutex");
	}
	if (config.mode == "mutex" || config.mode == "both") {
		_run_toggle_read_case(config, true, "mutex_guarded");
	}
}

REGISTER_TEST_COMMAND("cowdata-toggle-race", &run_cowdata_toggle_race);

} // namespace TestCowDataPerf
