#include "value.h"

namespace Clox {

std::ostream& operator<<(std::ostream& out, const Value& value)
{
	std::visit([](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v <T, bool>)
				std::cout << std::boolalpha << arg << std::noboolalpha;
			else if constexpr (std::is_same_v<T, std::monostate>)
				std::cout << "nil";
			else if constexpr (std::is_same_v<T, double>)
				std::cout << arg;
			else if constexpr (std::is_same_v<T, Obj*>)
				std::cout << arg->to_string();
		}, value.value);
	return out;
}

} //Clox