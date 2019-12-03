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

struct GC;

struct AllocBase
{
protected:
	static GC* gc;

public:
	static void init(GC* value)
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

	using worker = std::allocator<T>;
	using worker_traits = std::allocator_traits<worker>;

	static worker a;

	constexpr Allocator()noexcept {}
	constexpr Allocator(const Allocator&)noexcept = default;
	template<typename U>
	constexpr Allocator(const Allocator<U>&) noexcept {}

	[[nodiscard]] constexpr T* allocate(std::size_t n);
	constexpr void deallocate(T* p, std::size_t n);
};

template<typename T>
inline typename Allocator<T>::worker Allocator<T>::a{};

using table = std::map<ObjString*, Value, std::less<ObjString*>, Allocator<std::pair<ObjString* const, Value>>>;

struct GC
{
	std::unique_ptr<Obj, ObjDeleter> objects = nullptr;
	std::set<ObjString*, std::less<ObjString*>, Allocator<ObjString*>> strings;
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
	void mark_array(const ValueArray<>& array);
	void mark_compiler_roots();
	void mark_object(Obj* ptr);
	void mark_table(const table& table);
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

template<typename T>
[[nodiscard]] constexpr T* Allocator<T>::allocate(std::size_t n)
{
	if (gc != nullptr)
		gc->bytes_allocated += sizeof(T) * n;
	return worker_traits::allocate(a, n);
}

template<typename T>
constexpr void Allocator<T>::deallocate(T* p, std::size_t n)
{
	if (gc != nullptr)
		gc->bytes_allocated -= sizeof(T) * n;
	worker_traits::deallocate(a, p, n);
}

template<typename T, typename U>
[[nodiscard]] bool operator==(const Allocator<T>& t, const Allocator<U>& u)noexcept
{
	return std::is_same_v<T, U>;
}

template<typename T, typename U>
[[nodiscard]] bool operator!=(const Allocator<T>& t, const Allocator<U>& u)noexcept
{
	return !(t == u);
}

} //Clox