#pragma once

#include <string_view>

#include "chunk.h"
#include "obj.h"
#include "table.h"

namespace Clox {

struct GC;

struct ObjFunction;
struct ObjUpvalue;

std::string_view nameof(ObjType type);
std::ostream& operator<<(std::ostream& out, const Obj& obj);
void register_obj(std::unique_ptr<Obj, ObjDeleter>& obj, GC& gc)noexcept;

template<typename T, template<typename>typename Alloc = Allocator>
auto delete_obj(Alloc<T>& a, T* ptr)
->typename std::enable_if_t<std::is_base_of_v<Obj, T>, void>
{
	AllocTraits<T>::destroy(a, ptr);
	AllocTraits<T>::deallocate(a, ptr, 1);
}

struct ObjClass final :public Obj
{
	constexpr static auto obj_type = ObjType::Class;

	ObjString* const name;

	constexpr ObjClass(ObjString* name) noexcept
		:Obj(ObjType::Class), name(name)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjClass& c);

struct ObjClosure final :public Obj
{
	constexpr static auto obj_type = ObjType::Closure;

	ObjFunction* const function;
	std::vector<ObjUpvalue*, Allocator<ObjUpvalue*>> upvalues;

	ObjClosure(ObjFunction* func);

	constexpr size_t upvalue_count()const noexcept { return upvalues.size(); }
};
std::ostream& operator<<(std::ostream& out, const ObjClosure& s);

struct ObjFunction final : public Obj
{
	constexpr static auto obj_type = ObjType::Function;

	size_t arity = 0;
	size_t upvalue_count = 0;
	Chunk chunk;
	ObjString* name = nullptr;

	ObjFunction() :Obj(ObjType::Function) {}
};
std::ostream& operator<<(std::ostream& out, const ObjFunction& f);

struct ObjInstance final :public Obj
{
	constexpr static auto obj_type = ObjType::Instance;

	ObjClass* const klass;
	table fields;

	ObjInstance(ObjClass* klass)
		:Obj(ObjType::Instance), klass(klass)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjInstance& ins);

using NativeFn = Value(*)(uint8_t arg_count, Value * args);

struct ObjNative final :public Obj
{
	constexpr static auto obj_type = ObjType::Native;

	NativeFn function;

	constexpr ObjNative(NativeFn func)noexcept
		:Obj(ObjType::Native), function(func)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjNative& s);

struct ObjUpvalue final :public Obj
{
	constexpr static auto obj_type = ObjType::Upvalue;

	Value* location;
	Value closed;
	ObjUpvalue* next = nullptr;

	constexpr ObjUpvalue(Value* slot)noexcept
		:Obj(ObjType::Upvalue), location(slot)
	{
	}
};
std::ostream& operator<<(std::ostream& out, const ObjUpvalue& s);

template<typename T, template<typename> typename Alloc = Allocator, typename... Args>
[[nodiscard]] auto alloc_unique_obj(Args&&... args)
->typename std::enable_if_t<std::is_base_of_v<Obj, T>, std::unique_ptr<Obj, ObjDeleter>>
{
	static Alloc<T> a;

	auto p = AllocTraits<T>::allocate(a, 1);
	AllocTraits<T>::construct(a, p, std::forward<Args>(args)...);

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
	register_obj(p, gc);
	return static_cast<T*>(res);
}

} //Clox