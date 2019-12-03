#include "object.h"

#include "memory.h"
#include "obj_string.h"

namespace Clox {

ObjClosure::ObjClosure(ObjFunction* func)
	:ObjT(ObjType::Closure), function(func), upvalues(func->upvalue_count, nullptr)
{
}

std::ostream& operator<<(std::ostream& out, const ObjClosure& s)
{
	out << *s.function;
	return out;
}

ObjClosure* create_obj_closure(ObjFunction* func, GC& gc)
{
	auto p = new ObjClosure(func);
	register_obj(p, gc);
	return p;
}

std::ostream& operator<<(std::ostream& out, const ObjFunction& f)
{
	if (f.name == nullptr)
		out << "<script>";
	else
		out << "<fn " << *f.name << ">";
	return out;
}

ObjFunction* create_obj_function(GC& vm)
{
	auto p = new ObjFunction();
	register_obj(p, vm);
	return p;
}

std::ostream& operator<<(std::ostream& out, [[maybe_unused]] const ObjNative& s)
{
	out << "<native fn>";
	return out;
}

ObjNative* create_obj_native(NativeFn func, GC& gc)
{
	auto p = new ObjNative(func);
	register_obj(p, gc);
	return p;
}

std::ostream& operator<<(std::ostream& out, [[maybe_unused]] const ObjUpvalue& s)
{
	out << "upvalue";
	return out;
}

ObjUpvalue* create_obj_upvalue(Value* slot, GC& gc)
{
	auto p = new ObjUpvalue(slot);
	register_obj(p, gc);
	return p;
}

} // Clox