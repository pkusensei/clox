#pragma once

#include <functional>
#include <memory>

namespace Clox {

struct GC;

struct Obj;

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
	bool is_marked = false;
	std::unique_ptr<Obj, ObjDeleter> next = nullptr;

	constexpr bool is_type(ObjType type) const noexcept
	{
		return this->type == type;
	}

protected:
	constexpr Obj(ObjType type) noexcept :type(type) {}
};

std::ostream& operator<<(std::ostream& out, const Obj& obj);
void register_obj(Obj* obj, GC& gc);

} //Clox