#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <stack>

#include "obj.h"
#include "value.h"

#ifdef _DEBUG
#define DEBUG_LOG_GC
#endif // _DEBUG

#ifdef DEBUG_LOG_GC
#include"debug.h"
#endif // DEBUG_LOG_GC

namespace Clox {

struct ObjString;
struct VM;

template<typename T>
struct Allocator;

struct GC
{
	std::unique_ptr<Obj, ObjDeleter> objects = nullptr;
	std::set<ObjString*> strings;
	std::stack<Obj*> gray_stack;

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
	void mark_array(const ValueArray<Allocator<Value>>& array);
	void mark_compiler_roots();
	void mark_object(Obj* ptr);
	void mark_table(const std::map<ObjString*, Value>& table);
	void mark_value(const Value& value);

	void trace_references();
	void blacken_object(Obj* ptr);
	void remove_white_string();

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

struct AllocBase
{
protected:
	static GC* gc;

public:
	static void init_gc(GC* value)
	{
		if (gc == nullptr && value != nullptr)
			gc = value;
	}
};

inline GC* AllocBase::gc = nullptr;

template<typename T>
struct Allocator :public AllocBase
{
	static_assert(!std::is_const_v<T>);

	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using propagate_on_container_move_assignment = std::true_type;
	using is_always_equal = std::true_type;

	using worker = std::allocator<T>;

	worker a;

	constexpr Allocator()noexcept {}
	constexpr Allocator(const Allocator&)noexcept = default;
	template<typename U>
	constexpr Allocator(const Allocator<U>&) noexcept {}

	[[nodiscard]] constexpr T* allocate(std::size_t n)
	{
		gc->bytes_allocated += sizeof(T) * n;
		return std::allocator_traits<worker>::allocate(a, n);
	}

	constexpr void deallocate(T* p, std::size_t n)
	{
		gc->bytes_allocated -= sizeof(T) * n;
		std::allocator_traits<worker>::deallocate(a, p, n);
	}
};

} //Clox