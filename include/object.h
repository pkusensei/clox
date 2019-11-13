#pragma once

#include <memory>
#include <string_view>

#include "chunk.h"

namespace Clox {

struct Obj;
struct VM;

enum class ObjType
{
	Closure,
	Function,
	Native,
	String,
	Upvalue
};

struct ObjDeleter
{
	void operator()(Obj* ptr) const;
};

struct Obj
{
	ObjType type;
	std::unique_ptr<Obj, ObjDeleter> next = nullptr;

	constexpr bool is_type(ObjType type) const noexcept
	{
		return this->type == type;
	}

protected:
	constexpr Obj(ObjType type) noexcept :type(type) {}
};

std::ostream& operator<<(std::ostream& out, const Obj& obj);
void register_obj(Obj* obj, VM& vm);

template<typename Derived>
struct ObjT :public Obj
{
protected:
	constexpr explicit ObjT(ObjType type) noexcept :Obj(type) {}
	const Derived& as()const noexcept { return static_cast<const Derived&>(*this); }
};

struct ObjFunction final : public ObjT<ObjFunction>
{
	size_t arity = 0;
	size_t upvalue_count = 0;
	Chunk chunk;
	ObjString* name = nullptr;

	ObjFunction() :ObjT(ObjType::Function) {}
};

std::ostream& operator<<(std::ostream& out, const ObjFunction& f);
[[nodiscard]] ObjFunction* create_obj_function(VM& vm);

struct ObjUpvalue;
struct ObjClosure final :public ObjT<ObjClosure>
{
	ObjFunction* const function;
	std::vector<ObjUpvalue*> upvalues;

	ObjClosure(ObjFunction* func)
		:ObjT(ObjType::Closure), function(func), upvalues(func->upvalue_count, nullptr)
	{
	}

	constexpr size_t upvalue_count()const noexcept { return upvalues.size(); }
};

std::ostream& operator<<(std::ostream& out, const ObjClosure& s);
[[nodiscard]] ObjClosure* create_obj_closure(ObjFunction* func, VM& vm);

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
[[nodiscard]] ObjNative* create_obj_native(NativeFn func, VM& vm);

struct ObjString final :public ObjT<ObjString>
{
	std::string content;

	ObjString()noexcept :ObjT(ObjType::String) {}
	std::string_view text()const { return content; }
};

std::ostream& operator<<(std::ostream& out, const ObjString& s);
[[nodiscard]] std::string operator+(const ObjString& lhs, const ObjString& rhs);

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
[[nodiscard]] ObjUpvalue* create_obj_upvalue(Value* slot, VM& vm);

} //Clox