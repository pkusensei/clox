#pragma once

#include <memory>
#include <string_view>

//#include "vm.h"

namespace Clox {

struct Obj;
struct VM;

Obj* create_obj(Obj* obj, VM& vm);

enum class ObjType
{
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
	virtual std::string_view to_string()const = 0;

protected:
	constexpr Obj(ObjType type) noexcept
		:type(type)
	{
	}
};

template<typename Derived>
struct ObjT :public Obj
{
	constexpr explicit ObjT(ObjType type) noexcept :Obj(type) {}
	const Derived& as()const noexcept { return static_cast<const Derived&>(*this); }
	std::string_view to_string()const final
	{
		return as().text();
	}
};

struct ObjString final :public ObjT<ObjString>
{
	std::string content;

	ObjString() :ObjT(ObjType::String) {}
	std::string_view text()const { return content; }
};

std::string operator+(const ObjString& lhs, const ObjString& rhs);

} //Clox