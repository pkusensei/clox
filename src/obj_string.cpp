#include "obj_string.h"

namespace Clox {

std::ostream& operator<<(std::ostream& out, const ObjString& s)
{
	out << s.content;
	return out;
}

std::string operator+(const ObjString& lhs, const ObjString& rhs)
{
	return lhs.content + rhs.content;
}

} //Clox