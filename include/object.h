#pragma once

#include <memory>
#include <string_view>

#include "chunk.h"

namespace Clox {

struct Obj;
struct VM;

enum class ObjType
{
	Function,
	String
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
	virtual void to_ostream(std::ostream& out)const = 0;

protected:
	constexpr Obj(ObjType type) noexcept
		:type(type)
	{
	}
};

void create_obj(Obj* obj, VM& vm);

template<typename Derived>
struct ObjT :public Obj
{
	constexpr explicit ObjT(ObjType type) noexcept :Obj(type) {}
	const Derived& as()const noexcept { return static_cast<const Derived&>(*this); }
	void to_ostream(std::ostream& out)const final
	{
		out << as();
	}
};

struct ObjFunction final : public ObjT<ObjFunction>
{
	size_t arity = 0;
	Chunk chunk;
	ObjString* name = nullptr;

	ObjFunction() :ObjT(ObjType::Function) {}
};

std::ostream& operator<<(std::ostream& out, const ObjFunction& f);
ObjFunction* create_obj_function(VM& vm);

struct ObjString final :public ObjT<ObjString>
{
	std::string content;

	ObjString() :ObjT(ObjType::String) {}
	std::string_view text()const { return content; }
};

std::ostream& operator<<(std::ostream& out, const ObjString& s);
std::string operator+(const ObjString& lhs, const ObjString& rhs);

} //Clox