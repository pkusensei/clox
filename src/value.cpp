#include "value.h"

#include "object.h"
#include "obj_string.h"

namespace Clox {

std::ostream& operator<<(std::ostream& out, const Value& value)
{
#ifdef NAN_BOXING

	if (value.is_bool())
		out << std::boolalpha << value.as<bool>() << std::noboolalpha;
	else if (value.is_nil())
		out << "nil";
	else if (value.is_number())
		out << value.as<double>();
	else if (value.is_obj())
		out << *value.as<Obj*>();

#else
	std::visit([&out](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v <T, bool>)
				std::cout << std::boolalpha << arg << std::noboolalpha;
			else if constexpr (std::is_same_v<T, std::monostate>)
				std::cout << "nil";
			else if constexpr (std::is_same_v<T, double>)
				std::cout << arg;
			else if constexpr (std::is_same_v<T, Obj*>)
				out << *arg;
		}, value.value);
#endif // NAN_BOXING

	return out;
}

} //Clox