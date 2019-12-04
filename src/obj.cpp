#include "obj.h"

#include "object.h"
#include "obj_string.h"

namespace Clox {

void ObjDeleter::operator()(Obj* ptr) const
{

#define DEALLOC(t) \
do { \
auto p = static_cast<t*>(ptr); \
static Allocator<t> a; \
AllocTraits<t>::destroy(a, p); \
AllocTraits<t>::deallocate(a, p, 1); \
break; \
} while(false);

	switch (ptr->type)
	{
		case ObjType::Closure:
			DEALLOC(ObjClosure);
		case ObjType::Function:
			DEALLOC(ObjFunction);
		case ObjType::Native:
			DEALLOC(ObjNative);
		case ObjType::String:
			DEALLOC(ObjString);
		case ObjType::Upvalue:
			DEALLOC(ObjUpvalue);
		default:
			break;
	}
#undef DEALLOC
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