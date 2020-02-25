#pragma once

#include <functional>
#include <memory>

namespace Clox {

struct Obj;

enum class ObjType
{
	BoundMethod,
	Class,
	Closure,
	Function,
	Instance,
	Native,
	String,
	Upvalue
};

using ObjDeleter = std::function<void(Obj*)>;

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

} //Clox