/**************************************************************************/
/*  memory.cpp                                                            */
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

#include "memory.h"

#include "core/os/mutex.h"
#include "core/profiling/profiling.h"
#include "core/templates/safe_refcount.h"

#ifdef DEV_ENABLED
#include "core/math/math_funcs_binary.h"
#endif

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <unordered_map>

#ifdef DEBUG_ENABLED
static SafeNumeric<uint64_t> _current_mem_usage;
static SafeNumeric<uint64_t> _max_mem_usage;
#endif

void *operator new(size_t p_size, DefaultAllocator p_allocator) {
	return Memory::alloc_static(p_size);
}

void *operator new(size_t p_size, void *(*p_allocfunc)(size_t p_size)) {
	return p_allocfunc(p_size);
}

#ifdef DEBUG_ENABLED
struct AllocationProfileBlock {
	uintptr_t callsite = 0;
	size_t bytes = 0;
};

static Mutex _allocation_profile_mutex;
static std::unordered_map<void *, AllocationProfileBlock> _allocation_profile_blocks;
static std::unordered_map<uintptr_t, Memory::AllocationProfileRecord> _allocation_profile_sites;
static std::atomic<bool> _allocation_profile_enabled{ false };
static thread_local bool _allocation_profile_recursing = false;
static std::atomic<bool> _allocation_profile_runtime_ready{ false };

// File-scope Godot String/HashMap objects can allocate while the dynamic
// initializers for this translation unit are still running. Keep profiling
// disabled until its mutex and tracking maps are constructed, and disable it
// before those objects are destroyed at process shutdown. This preserves
// cleanup while profiling is paused without touching an unconstructed mutex.
struct AllocationProfileRuntimeLifetime {
	AllocationProfileRuntimeLifetime() {
		_allocation_profile_runtime_ready.store(true, std::memory_order_release);
	}

	~AllocationProfileRuntimeLifetime() {
		_allocation_profile_runtime_ready.store(false, std::memory_order_release);
	}
};

static AllocationProfileRuntimeLifetime _allocation_profile_runtime_lifetime;

static _FORCE_INLINE_ bool _allocation_profile_capture_enabled() {
	return _allocation_profile_runtime_ready.load(std::memory_order_acquire) && _allocation_profile_enabled.load(std::memory_order_relaxed) && !_allocation_profile_recursing;
}

static void _profile_alloc(void *p_ptr, size_t p_bytes, uintptr_t p_callsite) {
	if (!p_ptr || !_allocation_profile_runtime_ready.load(std::memory_order_acquire) || !_allocation_profile_enabled.load(std::memory_order_relaxed) || _allocation_profile_recursing || p_callsite == 0) {
		return;
	}
	_allocation_profile_recursing = true;
	{
		MutexLock lock(_allocation_profile_mutex);
		if (_allocation_profile_enabled.load(std::memory_order_relaxed)) {
			AllocationProfileBlock block;
			block.callsite = p_callsite;
			block.bytes = p_bytes;
			_allocation_profile_blocks[p_ptr] = block;
			Memory::AllocationProfileRecord &record = _allocation_profile_sites[p_callsite];
			record.callsite = p_callsite;
			record.live_bytes += p_bytes;
			record.live_blocks += 1;
			record.allocation_count += 1;
			record.peak_live_bytes = MAX(record.peak_live_bytes, record.live_bytes);
		}
	}
	_allocation_profile_recursing = false;
}

static void _profile_free(void *p_ptr) {
	if (!p_ptr || !_allocation_profile_runtime_ready.load(std::memory_order_acquire) || _allocation_profile_recursing) {
		return;
	}
	_allocation_profile_recursing = true;
	{
		MutexLock lock(_allocation_profile_mutex);
		const auto block_it = _allocation_profile_blocks.find(p_ptr);
		if (block_it != _allocation_profile_blocks.end()) {
			const AllocationProfileBlock block = block_it->second;
			const auto site_it = _allocation_profile_sites.find(block.callsite);
			if (site_it != _allocation_profile_sites.end()) {
				Memory::AllocationProfileRecord &record = site_it->second;
				record.live_bytes -= MIN(record.live_bytes, (uint64_t)block.bytes);
				record.live_blocks -= MIN(record.live_blocks, (uint64_t)1);
				record.free_count += 1;
			}
			_allocation_profile_blocks.erase(block_it);
		}
	}
	_allocation_profile_recursing = false;
}

static void _profile_realloc(void *p_old_ptr, void *p_new_ptr, size_t p_new_bytes, uintptr_t p_callsite) {
	if (!_allocation_profile_runtime_ready.load(std::memory_order_acquire) || _allocation_profile_recursing) {
		return;
	}
	_allocation_profile_recursing = true;
	{
		MutexLock lock(_allocation_profile_mutex);
		const auto block_it = _allocation_profile_blocks.find(p_old_ptr);
		if (block_it != _allocation_profile_blocks.end()) {
			const AllocationProfileBlock block = block_it->second;
			const auto site_it = _allocation_profile_sites.find(block.callsite);
			if (site_it != _allocation_profile_sites.end()) {
				Memory::AllocationProfileRecord &record = site_it->second;
				record.live_bytes -= MIN(record.live_bytes, (uint64_t)block.bytes);
				record.live_blocks -= MIN(record.live_blocks, (uint64_t)1);
				record.free_count += 1;
			}
			_allocation_profile_blocks.erase(block_it);
		}
		if (p_new_ptr && _allocation_profile_enabled.load(std::memory_order_relaxed) && p_callsite != 0) {
			AllocationProfileBlock block;
			block.callsite = p_callsite;
			block.bytes = p_new_bytes;
			_allocation_profile_blocks[p_new_ptr] = block;
			Memory::AllocationProfileRecord &record = _allocation_profile_sites[p_callsite];
			record.callsite = p_callsite;
			record.live_bytes += p_new_bytes;
			record.live_blocks += 1;
			record.allocation_count += 1;
			record.peak_live_bytes = MAX(record.peak_live_bytes, record.live_bytes);
		}
	}
	_allocation_profile_recursing = false;
}
#else
static _FORCE_INLINE_ bool _allocation_profile_capture_enabled() {
	return false;
}

static void _profile_alloc(void *, size_t, uintptr_t) {}
static void _profile_free(void *) {}
static void _profile_realloc(void *, void *, size_t, uintptr_t) {}
#endif

void *Memory::alloc_aligned_static(size_t p_bytes, size_t p_alignment) {
	DEV_ASSERT(Math::is_power_of_2(p_alignment));

	void *p1, *p2;
	if ((p1 = (void *)malloc(p_bytes + p_alignment - 1 + sizeof(uint32_t))) == nullptr) {
		return nullptr;
	}
	if (_allocation_profile_capture_enabled()) {
		_profile_alloc(p1, p_bytes + p_alignment - 1 + sizeof(uint32_t), (uintptr_t)__builtin_return_address(0));
	}
	GodotProfileAlloc(p1, p_bytes + p_alignment - 1 + sizeof(uint32_t));

	p2 = (void *)(((uintptr_t)p1 + sizeof(uint32_t) + p_alignment - 1) & ~((p_alignment)-1));
	*((uint32_t *)p2 - 1) = (uint32_t)((uintptr_t)p2 - (uintptr_t)p1);
	return p2;
}

void *Memory::realloc_aligned_static(void *p_memory, size_t p_bytes, size_t p_prev_bytes, size_t p_alignment) {
	if (p_memory == nullptr) {
		return alloc_aligned_static(p_bytes, p_alignment);
	}

	void *ret = alloc_aligned_static(p_bytes, p_alignment);
	if (ret) {
		memcpy(ret, p_memory, p_prev_bytes);
	}
	free_aligned_static(p_memory);
	return ret;
}

void Memory::free_aligned_static(void *p_memory) {
	uint32_t offset = *((uint32_t *)p_memory - 1);
	void *p = (void *)((uint8_t *)p_memory - offset);
	_profile_free(p);
	GodotProfileFree(p);
	free(p);
}

template <bool p_ensure_zero>
void *Memory::alloc_static(size_t p_bytes, bool p_pad_align) {
#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	void *mem;
	if constexpr (p_ensure_zero) {
		mem = calloc(1, p_bytes + (prepad ? DATA_OFFSET : 0));
	} else {
		mem = malloc(p_bytes + (prepad ? DATA_OFFSET : 0));
	}

	ERR_FAIL_NULL_V(mem, nullptr);
	if (_allocation_profile_capture_enabled()) {
		_profile_alloc(mem, p_bytes + (prepad ? DATA_OFFSET : 0), (uintptr_t)__builtin_return_address(0));
	}
	GodotProfileAlloc(mem, p_bytes + (prepad ? DATA_OFFSET : 0));

	if (prepad) {
		uint8_t *s8 = (uint8_t *)mem;

		uint64_t *s = (uint64_t *)(s8 + SIZE_OFFSET);
		*s = p_bytes;

#ifdef DEBUG_ENABLED
		uint64_t new_mem_usage = _current_mem_usage.add(p_bytes);
		_max_mem_usage.exchange_if_greater(new_mem_usage);
#endif
		return s8 + DATA_OFFSET;
	} else {
		return mem;
	}
}

template void *Memory::alloc_static<true>(size_t p_bytes, bool p_pad_align);
template void *Memory::alloc_static<false>(size_t p_bytes, bool p_pad_align);

void *Memory::realloc_static(void *p_memory, size_t p_bytes, bool p_pad_align) {
	if (p_memory == nullptr) {
		return alloc_static(p_bytes, p_pad_align);
	}

	uint8_t *mem = (uint8_t *)p_memory;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;
		uint64_t *s = (uint64_t *)(mem + SIZE_OFFSET);

#ifdef DEBUG_ENABLED
		if (p_bytes > *s) {
			uint64_t new_mem_usage = _current_mem_usage.add(p_bytes - *s);
			_max_mem_usage.exchange_if_greater(new_mem_usage);
		} else {
			_current_mem_usage.sub(*s - p_bytes);
		}
#endif

		if (p_bytes == 0) {
			_profile_free(mem);
			GodotProfileFree(mem);
			free(mem);
			return nullptr;
		} else {
			*s = p_bytes;

			uint8_t *old_mem = mem;
			mem = (uint8_t *)realloc(mem, p_bytes + DATA_OFFSET);
			ERR_FAIL_NULL_V(mem, nullptr);
			if (_allocation_profile_capture_enabled()) {
				_profile_realloc(old_mem, mem, p_bytes + DATA_OFFSET, (uintptr_t)__builtin_return_address(0));
			} else {
				_profile_realloc(old_mem, mem, p_bytes + DATA_OFFSET, 0);
			}
			GodotProfileFree(old_mem);
			GodotProfileAlloc(mem, p_bytes + DATA_OFFSET);

			s = (uint64_t *)(mem + SIZE_OFFSET);

			*s = p_bytes;

			return mem + DATA_OFFSET;
		}
	} else {
		uint8_t *old_mem = mem;
		mem = (uint8_t *)realloc(mem, p_bytes);

		ERR_FAIL_COND_V(mem == nullptr && p_bytes > 0, nullptr);
		if (_allocation_profile_capture_enabled()) {
			_profile_realloc(old_mem, mem, p_bytes, (uintptr_t)__builtin_return_address(0));
		} else {
			_profile_realloc(old_mem, mem, p_bytes, 0);
		}
		GodotProfileFree(old_mem);
		GodotProfileAlloc(mem, p_bytes);

		return mem;
	}
}

void Memory::free_static(void *p_ptr, bool p_pad_align) {
	ERR_FAIL_NULL(p_ptr);

	uint8_t *mem = (uint8_t *)p_ptr;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;

#ifdef DEBUG_ENABLED
		uint64_t *s = (uint64_t *)(mem + SIZE_OFFSET);
		_current_mem_usage.sub(*s);
#endif

		_profile_free(mem);
		GodotProfileFree(mem);
		free(mem);
	} else {
		_profile_free(mem);
		GodotProfileFree(mem);
		free(mem);
	}
}

uint64_t Memory::get_mem_available() {
	return -1; // 0xFFFF...
}

uint64_t Memory::get_mem_usage() {
#ifdef DEBUG_ENABLED
	return _current_mem_usage.get();
#else
	return 0;
#endif
}

uint64_t Memory::get_mem_max_usage() {
#ifdef DEBUG_ENABLED
	return _max_mem_usage.get();
#else
	return 0;
#endif
}

void Memory::set_allocation_profile_enabled(bool p_enabled) {
#ifdef DEBUG_ENABLED
	if (!_allocation_profile_runtime_ready.load(std::memory_order_acquire)) {
		return;
	}
	MutexLock lock(_allocation_profile_mutex);
	_allocation_profile_enabled.store(p_enabled, std::memory_order_release);
#else
	(void)p_enabled;
#endif
}

void Memory::reset_allocation_profile() {
#ifdef DEBUG_ENABLED
	if (!_allocation_profile_runtime_ready.load(std::memory_order_acquire)) {
		return;
	}
	MutexLock lock(_allocation_profile_mutex);
	_allocation_profile_recursing = true;
	_allocation_profile_blocks.clear();
	_allocation_profile_sites.clear();
	_allocation_profile_recursing = false;
#endif
}

bool Memory::is_allocation_profile_enabled() {
#ifdef DEBUG_ENABLED
	if (!_allocation_profile_runtime_ready.load(std::memory_order_acquire)) {
		return false;
	}
	MutexLock lock(_allocation_profile_mutex);
	return _allocation_profile_enabled.load(std::memory_order_acquire);
#else
	return false;
#endif
}

std::vector<Memory::AllocationProfileRecord> Memory::get_allocation_profile() {
	std::vector<Memory::AllocationProfileRecord> result;
#ifdef DEBUG_ENABLED
	if (!_allocation_profile_runtime_ready.load(std::memory_order_acquire)) {
		return result;
	}
	_allocation_profile_recursing = true;
	{
		MutexLock lock(_allocation_profile_mutex);
		result.reserve(_allocation_profile_sites.size());
		for (const auto &entry : _allocation_profile_sites) {
			result.push_back(entry.second);
		}
	}
	std::sort(result.begin(), result.end(), [](const Memory::AllocationProfileRecord &p_left, const Memory::AllocationProfileRecord &p_right) {
		if (p_left.live_bytes != p_right.live_bytes) {
			return p_left.live_bytes > p_right.live_bytes;
		}
		return p_left.peak_live_bytes > p_right.peak_live_bytes;
	});
	_allocation_profile_recursing = false;
#endif
	return result;
}

_GlobalNil::_GlobalNil() {
	left = this;
	right = this;
	parent = this;
}

_GlobalNil _GlobalNilClass::_nil;
