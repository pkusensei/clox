#pragma once

#include <string_view>

#include "chunk.h"
#include "obj.h"
#include "value.h"

namespace Clox {

struct GC;

struct ObjFunction;
struct ObjUpvalue;

struct ObjClosure final :public ObjT<ObjClosure>
{
	ObjFunction* const function;
	std::vector<ObjUpvalue*> upvalues;

	ObjClosure(ObjFunction* func);

	constexpr size_t upvalue_count()const noexcept { return upvalues.size(); }
};

std::ostream& operator<<(std::ostream& out, const ObjClosure& s);
[[nodiscard]] ObjClosure* create_obj_closure(ObjFunction* func, GC& gc);

struct ObjFunction final : public ObjT<ObjFunction>
{
	size_t arity = 0;
	size_t upvalue_count = 0;
	Chunk chunk;
	ObjString* name = nullptr;

	ObjFunction() :ObjT(ObjType::Function) {}
};

std::ostream& operator<<(std::ostream& out, const ObjFunction& f);
[[nodiscard]] ObjFunction* create_obj_function(GC& gc);

using NativeFn = Value(*)(uint8_t arg_count, Value * args);

struct ObjNative final :public ObjT<ObjNative>
{
	NativeFn function;

	constexpr ObjNative(NativeFn func)noexcept
		:ObjT(ObjType::Native), function(func)
	{
	}
};

std::ostream& operator<<(std::ostream& out, const ObjNative& s);
[[nodiscard]] ObjNative* create_obj_native(NativeFn func, GC& gc);

struct ObjUpvalue final :public ObjT<ObjUpvalue>
{
	Value* location;
	Value closed;
	ObjUpvalue* next = nullptr;

	constexpr ObjUpvalue(Value* slot)noexcept
		:ObjT(ObjType::Upvalue), location(slot)
	{
	}
};

std::ostream& operator<<(std::ostream& out, const ObjUpvalue& s);
[[nodiscard]] ObjUpvalue* create_obj_upvalue(Value* slot, GC& gc);

} //Clox