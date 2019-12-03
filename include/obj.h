#pragma once

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

// TODO try extract Obj and ObjDeleter to another file to solve cyclic dependency
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

template<typename Derived>
struct ObjT :public Obj
{
protected:
	constexpr explicit ObjT(ObjType type) noexcept :Obj(type) {}
	const Derived& as()const noexcept { return static_cast<const Derived&>(*this); }
};

} //Clox