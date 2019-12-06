cmake_minimum_required (VERSION 3.8)

project(clox LANGUAGES CXX)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_executable (${PROJECT_NAME} src/clox.cpp
	src/chunk.cpp
	src/compiler.cpp
	src/debug.cpp
	src/memory.cpp
	src/object.cpp
	src/obj_string.cpp
	src/scanner.cpp
	src/value.cpp
	src/vm.cpp)

target_include_directories(${PROJECT_NAME} 
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)