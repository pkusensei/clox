#pragma once

#include <algorithm>
#include <deque>
#include <memory>
#include <set>

#include "obj.h"
#include "table.h"

#ifdef _DEBUG
#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC
#endif // _DEBUG

namespace Clox {

struct ObjString;
struct VM;

struct GC;

struct AllocBase
{
protected:
	inline static GC* gc = nullptr;

public:
	static void init(GC* value)noexcept
	{
		if (gc == nullptr && value != nullptr)
			gc = value;
	}
};

template<typename T>
struct Allocator final :public AllocBase
{
	static_assert(!std::is_const_v<T>);

	using value_type = T;

	inline static std::allocator<T> worker = {};
	using worker_traits = std::allocator_traits<decltype(worker)>;

	constexpr Allocator()noexcept {}
	constexpr Allocator(const Allocator&)noexcept = default;
	template<typename U>
	constexpr Allocator(const Allocator<U>&) noexcept {}

	[[nodiscard]] constexpr T* allocate(std::size_t n);
	constexpr void deallocate(T* p, std::size_t n)noexcept;
};

struct GC
{
	std::unique_ptr<Obj, ObjDeleter> objects = nullptr;
	std::set<ObjString*, std::less<ObjString*>, Allocator<ObjString*>> strings;
	std::deque<Obj*> gray_stack;

	size_t bytes_allocated = 0;
	size_t next_gc = 1024 * 1024;

	VM& vm;

	explicit GC(VM& vm)noexcept
		:vm(vm)
	{
	}

	void collect();

private:
	void mark_roots();
	void mark_array(const ValueArray<>& array);
	void mark_compiler_roots();
	void mark_object(Obj* const ptr);
	void mark_table(const table& table);
	void mark_value(const Value& value);

	void trace_references();
	void blacken_object(Obj* ptr);
	void remove_white_string()noexcept;

	void sweep();

public:
	template<typename T>
	[[nodiscard]] ObjString* find_string(const T& str)const
	{
		if (strings.empty()) return nullptr;

		auto res = std::find_if(strings.cbegin(), strings.cend(),
			[&str](const auto& it) { return it->content == str; });
		if (res != strings.end())
			return *res;
		return nullptr;
	}

};

template<typename T>
[[nodiscard]] constexpr T* Allocator<T>::allocate(std::size_t n)
{
	auto alloc_size = n * sizeof(T);
	auto p = worker_traits::allocate(worker, n);

	if (gc != nullptr)
	{
		gc->bytes_allocated += alloc_size;

#ifdef DEBUG_STRESS_GC
		gc->collect();
#else
		if (gc->bytes_allocated > gc->next_gc)
			gc->collect();
#endif // DEBUG_STRESS_GC

	}
	return p;
}

template<typename T>
constexpr void Allocator<T>::deallocate(T* p, std::size_t n)noexcept
{
	worker_traits::deallocate(worker, p, n);
	if (gc != nullptr)
		gc->bytes_allocated -= sizeof(T) * n;
}

template<typename T, typename U>
[[nodiscard]] bool operator==(const Allocator<T>& t, const Allocator<U>& u)noexcept
{
	return true;
}

template<typename T, typename U>
[[nodiscard]] bool operator!=(const Allocator<T>& t, const Allocator<U>& u)noexcept
{
	return false;
}

} //Clox