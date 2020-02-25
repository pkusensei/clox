#pragma once

#include <string_view>

#include "chunk.h"
#include "obj.h"
#include "table.h"

namespace Clox {

struct GC;

std::ostream& operator<<(std::ostream& out, const Obj& obj);
void register_obj(std::unique_ptr<Obj, ObjDeleter>&& obj, GC& gc)noexcept;

template<typename T, template<typename>typename Alloc = Allocator>
auto delete_obj(Alloc<T>& a, T* ptr)
->typename std::enable_if_t<std::is_base_of_v<Obj, T>, void>
{
	using AllocTraits = std::allocator_traits<Alloc<T>>;
	AllocTraits::destroy(a, ptr);
	AllocTraits::deallocate(a, ptr, 1);
}

struct ObjFunction final : public Obj
{
	size_t arity = 0;
	size_t upvalue_count = 0;
	Chunk chunk;
	ObjString* name = nullptr;

	ObjFunction() :Obj(ObjType::Function) {}
};
std::ostream& operator<<(std::ostream& out, const ObjFunction& f);

using NativeFn = Value(*)(uint8_t arg_count, Value * args);

struct ObjNative final :public Obj
{
	NativeFn function;

	constexpr explicit ObjNative(NativeFn func)noexcept
		:Obj(ObjType::Native), function(func)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjNative& s);

struct ObjUpvalue final :public Obj
{
	Value* location;
	Value closed;
	ObjUpvalue* next = nullptr;

	constexpr explicit ObjUpvalue(Value* slot)noexcept
		:Obj(ObjType::Upvalue), location(slot)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjUpvalue& s);

struct ObjClosure final :public Obj
{
	ObjFunction* const function;
	std::vector<ObjUpvalue*, Allocator<ObjUpvalue*>> upvalues;

	explicit ObjClosure(ObjFunction* func);

	constexpr size_t upvalue_count()const noexcept { return upvalues.size(); }
};
std::ostream& operator<<(std::ostream& out, const ObjClosure& s);

struct ObjClass final :public Obj
{
	ObjString* const name;
	table methods;

	ObjClass(ObjString* name) noexcept
		:Obj(ObjType::Class), name(name)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjClass& c);

struct ObjInstance final :public Obj
{
	ObjClass* const klass;
	table fields;

	explicit ObjInstance(ObjClass* klass)
		:Obj(ObjType::Instance), klass(klass)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjInstance& ins);

struct ObjBoundMethod :public Obj
{
	Value receiver;
	ObjClosure* const method;

	constexpr ObjBoundMethod(Value receiver, ObjClosure* const method)
		:Obj(ObjType::BoundMethod), receiver(std::move(receiver)), method(method)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjBoundMethod& bm);

template<typename T, template<typename> typename Alloc = Allocator, typename... Args>
[[nodiscard]] auto alloc_unique_obj(Args&&... args)
->typename std::enable_if_t<std::is_base_of_v<Obj, T>, std::unique_ptr<Obj, ObjDeleter>>
{
	static Alloc<T> a;

	using AllocTraits = std::allocator_traits<Alloc<T>>;
	auto p = AllocTraits::allocate(a, 1);
	AllocTraits::construct(a, p, std::forward<Args>(args)...);

#ifdef DEBUG_LOG_GC
	std::cout << (void*)p << " allocate " << sizeof(T);
	std::cout << " for " << nameof(p->type) << '\n';
#endif // DEBUG_LOG_GC

	auto deleter = [](Obj* ptr)
	{
#ifdef DEBUG_LOG_GC
		std::cout << (void*)ptr << " free type " << nameof(ptr->type) << '\n';
#endif // DEBUG_LOG_GC

		delete_obj(a, static_cast<T*>(ptr));
	};

	return { p, deleter };
}

template<typename T, typename... Args>
[[nodiscard]] auto create_obj(GC& gc, Args&&... args)
->typename std::enable_if_t<std::is_base_of_v<Obj, T>, T*>
{
	static_assert(std::is_constructible_v<T, Args...>);
	auto p = alloc_unique_obj<T>(std::forward<Args>(args)...);
	auto res = p.get();
	register_obj(std::move(p), gc);
	return static_cast<T*>(res);
}

template<typename T>
constexpr auto objtype_of()
->typename std::enable_if_t<std::is_base_of_v<Obj, T> && !std::is_same_v<Obj, T>, ObjType>
{
	if constexpr (std::is_same_v<T, ObjBoundMethod>)
		return ObjType::BoundMethod;
	else if constexpr (std::is_same_v<T, ObjClass>)
		return ObjType::Class;
	else if constexpr (std::is_same_v<T, ObjClosure>)
		return ObjType::Closure;
	else if constexpr (std::is_same_v<T, ObjFunction>)
		return ObjType::Function;
	else if constexpr (std::is_same_v<T, ObjInstance>)
		return ObjType::Instance;
	else if constexpr (std::is_same_v<T, ObjNative>)
		return ObjType::Native;
	else if constexpr (std::is_same_v<T, ObjString>)
		return ObjType::String;
	else if constexpr (std::is_same_v<T, ObjUpvalue>)
		return ObjType::Upvalue;
}

template<typename T>
constexpr auto nameof()
->typename std::enable_if_t<std::is_base_of_v<Obj, T> && !std::is_same_v<Obj, T>, std::string_view>
{
	using namespace std::literals;

	switch (objtype_of<T>())
	{
		case ObjType::BoundMethod:
			return "bound method"sv;
		case ObjType::Class:
			return "class"sv;
		case ObjType::Closure:
			return "closure"sv;
		case ObjType::Function:
			return "function"sv;
		case ObjType::Instance:
			return "instance"sv;
		case ObjType::Native:
			return "native function"sv;
		case ObjType::String:
			return "string"sv;
		case ObjType::Upvalue:
			return "upvalue"sv;
	}
}

} //Clox