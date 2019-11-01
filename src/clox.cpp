#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "vm.h"

namespace fs = std::filesystem;

void repl(Clox::VM& vm);
int run_file(Clox::VM& vm, fs::path path);

int main(int argc, char* argv[])
{
	Clox::VM vm;
	if (argc == 1)
		repl(vm);
	else if (argc == 2)
		return run_file(vm, argv[1]);
	else
	{
		std::cerr << "Usage: clox [path]\n";
		return 64;
	}
	return 0;
}

void repl(Clox::VM& vm)
{
	std::string line;
	while (true)
	{
		std::cout << "> ";
		if (!std::getline(std::cin, line))
		{
			std::cout << '\n';
			break;
		}
		vm.interpret(line);
		line.clear();
	}
}

int run_file(Clox::VM& vm, fs::path path)
{
	try
	{
		std::ifstream file(path);
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string source = buffer.str();
		auto result = vm.interpret(source);
		switch (result)
		{
			case Clox::InterpretResult::CompileError:
				return 65;
			case Clox::InterpretResult::RuntimeError:
				return 70;
			case Clox::InterpretResult::Ok:
			default:
				return 0;
		}
	} catch (...)
	{
		std::cout << "Could not open or read file \"" << path << "\".\n";
		return 74;
	}
}

