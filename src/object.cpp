#include "object.h"

#include "memory.h"
#include "obj_string.h"

namespace Clox {

std::ostream& operator<<(std::ostream& out, const Obj& obj)
{
	switch (obj.type)
	{
		case ObjType::Class:
			out << static_cast<const ObjClass&>(obj);
			break;
		case ObjType::Closure:
			out << static_cast<const ObjClosure&>(obj);
			break;
		case ObjType::Function:
			out << static_cast<const ObjFunction&>(obj);
			break;
		case ObjType::Instance:
			out << static_cast<const ObjInstance&>(obj);
			break;
		case ObjType::Native:
			out << static_cast<const ObjNative&>(obj);
			break;
		case ObjType::String:
			out << static_cast<const ObjString&>(obj);
			break;
		case ObjType::Upvalue:
			out << static_cast<const ObjUpvalue&>(obj);
			break;
		default:
			throw std::invalid_argument("Unexpected ObjType:: Output failed");
	}
	return out;
}

void register_obj(std::unique_ptr<Obj, ObjDeleter>&& obj, GC& gc)noexcept
{
	obj->next = std::move(gc.objects);
	gc.objects = std::move(obj);
}

ObjClosure::ObjClosure(ObjFunction* func)
	:Obj(ObjType::Closure), function(func), upvalues(func->upvalue_count, nullptr)
{
}

std::ostream& operator<<(std::ostream& out, const ObjClass& c)
{
	out << *c.name;
	return out;
}

std::ostream& operator<<(std::ostream& out, const ObjClosure& s)
{
	out << *s.function;
	return out;
}

std::ostream& operator<<(std::ostream& out, const ObjFunction& f)
{
	if (f.name == nullptr)
		out << "<script>";
	else
		out << "<fn " << *f.name << ">";
	return out;
}

std::ostream& operator<<(std::ostream& out, const ObjInstance& ins)
{
	out << *ins.klass << " instance";
	return out;
}

std::ostream& operator<<(std::ostream& out, const ObjBoundMethod& bm)
{
	out << *bm.method->function;
	return out;
}

std::ostream& operator<<(std::ostream& out, [[maybe_unused]] const ObjNative& s)
{
	out << "<native fn>";
	return out;
}

std::ostream& operator<<(std::ostream& out, [[maybe_unused]] const ObjUpvalue& s)
{
	out << "upvalue";
	return out;
}

} // Clox