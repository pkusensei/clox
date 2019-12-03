#include "obj.h"

#include "object.h"
#include "obj_string.h"

namespace Clox {

void ObjDeleter::operator()(Obj* ptr) const
{
	switch (ptr->type)
	{
		case ObjType::Closure:
			delete static_cast<ObjClosure*>(ptr);
			break;
		case ObjType::Function:
			delete static_cast<ObjFunction*>(ptr);
			break;
		case ObjType::Native:
			delete static_cast<ObjNative*>(ptr);
			break;
		case ObjType::String:
			delete static_cast<ObjString*>(ptr);
			break;
		case ObjType::Upvalue:
			delete static_cast<ObjUpvalue*>(ptr);
			break;

		default:
			break;
	}
}

std::ostream& operator<<(std::ostream& out, const Obj& obj)
{
	switch (obj.type)
	{
		case ObjType::Closure:
			out << static_cast<const ObjClosure&>(obj);
			break;
		case ObjType::Function:
			out << static_cast<const ObjFunction&>(obj);
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
			break;
	}
	return out;
}

void register_obj(Obj* obj, GC& gc)
{
	std::unique_ptr<Obj, ObjDeleter> p(obj, ObjDeleter{});
	p->next = std::move(gc.objects);
	gc.objects = std::move(p);
}

} //Clox