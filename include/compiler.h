#pragma once

#include <array>
#include <optional>

#include "chunk.h"
#include "scanner.h"

namespace Clox {

struct Chunk;
struct ObjFunction;
struct VM;

struct Compilation;
struct Parser;

enum class Precedence :uint8_t
{
	None,
	Assignment,  // =        
	Or,          // or       
	And,         // and      
	Equality,    // == !=    
	Comparison,  // < > <= >=
	Term,        // + -      
	Factor,      // * /      
	Unary,       // ! -      
	Call,        // . () []  
	Primary
};

inline Precedence operator+(Precedence prec, uint8_t i)noexcept
{
	return static_cast<Precedence>(static_cast<uint8_t>(prec) + i);
}

void error(Parser& parser, std::string_view message);
void error_at_current(Parser& parser, std::string_view message);
void error_at(Parser& parser, const Token& token, std::string_view message);

struct Parser
{
	Scanner scanner;
	Token current;
	Token previous;
	bool had_error = false;
	bool panic_mode = false;

	constexpr explicit Parser(std::string_view source)noexcept
		:scanner(source)
	{
	}

	void advance();
	[[nodiscard]] bool check(TokenType type)const noexcept;
	void consume(TokenType type, std::string_view message);
	[[nodiscard]] bool match(TokenType type);

	void synchronize();
};

using ParseFn = void (Compilation::*)(bool can_assign);
struct ParseRule
{
	ParseFn prefix = nullptr;
	ParseFn infix = nullptr;
	Precedence precedence;
};

enum class FunctionType
{
	Function,
	Initializer,
	Method,
	Script,
};

struct Local
{
	Token name;
	int depth = -1;
	bool is_captured = false;
};

struct Upvalue
{
	uint8_t index = 0;
	bool is_local = false;
};

struct Compiler
{
	std::unique_ptr<Compiler> enclosing = nullptr;
	ObjFunction* function = nullptr;
	FunctionType type = FunctionType::Script;

	std::array<Local, UINT8_COUNT> locals;
	size_t local_count = 0;
	std::array<Upvalue, UINT8_COUNT> upvalues;
	int scope_depth = 0;
};

struct ClassCompiler
{
	std::unique_ptr<ClassCompiler> enclosing = nullptr;
	Token name;
	bool has_superclass = false;
};

struct Compilation
{
	std::unique_ptr<Compiler> current = nullptr;
	std::unique_ptr<ClassCompiler> current_class = nullptr;
	std::unique_ptr<Parser> parser = nullptr;
	VM& vm;

	constexpr explicit Compilation(VM& vm)noexcept
		:vm(vm)
	{
	}

	[[nodiscard]] ObjFunction* compile(std::string_view source);

private:

	void expression();
	void and_(bool can_assign);
	void binary(bool can_assign);
	void call(bool can_assign);
	void dot(bool can_assign);
	void grouping(bool can_assign);
	void literal(bool can_assign);
	void number(bool can_assign);
	void or_(bool can_assign);
	void string(bool can_assign);
	void unary(bool can_assign);
	void variable(bool can_assign);
	void super_(bool can_assign);
	void this_(bool can_assign);

	void statement();
	void block();
	void expression_statement();
	void for_statement();
	void if_statement();
	void print_statement();
	void return_statement();
	void while_statement();

	void declaration();
	void class_declaration();
	void fun_declaration();
	void var_declaration();
	void function(FunctionType type);
	void method();

	[[nodiscard]] uint8_t argument_list();
	void declare_variable();
	void define_variable(uint8_t global)const;
	[[nodiscard]] uint8_t identifier_constant(const Token& name);
	void named_variable(const Token& name, bool can_assign);
	void parse_precedence(Precedence precedence);
	[[nodiscard]] uint8_t parse_variable(std::string_view error);

	void init_compiler(FunctionType type);
	[[nodiscard]] auto end_compiler()->std::pair<ObjFunction*, std::unique_ptr<Compiler>>;
	void add_local(const Token& name);
	[[nodiscard]] uint8_t add_upvalue(const std::unique_ptr<Compiler>& compiler, uint8_t index, bool is_local);
	void begin_scope()const;
	void end_scope()const;
	void mark_initializied()const;
	[[nodiscard]] std::optional<uint8_t> resolve_local(const std::unique_ptr<Compiler>& compiler, const Token& name);
	[[nodiscard]] std::optional<uint8_t> resolve_upvalue(const std::unique_ptr<Compiler>& compiler, const Token& name);

	template<typename T>
	[[nodiscard]] typename std::enable_if_t<std::is_convertible_v<T, Value>, uint8_t>
		make_constant(T&& value)
	{
		vm.push(value);
		auto constant = current_chunk().add_constant(std::forward<T>(value));
		vm.pop();
		if (constant > UINT8_MAX)
		{
			error(*parser, "Too many constants in one chunk.");
			return 0;
		}
		return static_cast<uint8_t>(constant);
	}

	template<typename T>
	typename std::enable_if_t<opcode_trait_v<T>, void>
		emit_byte(T byte) const
	{
		current_chunk().write(byte, parser->previous.line);
	}

	template<typename T, typename... Ts>
	typename std::enable_if_t<opcode_trait_v<T, Ts...>, void>
		emit_byte(T byte, Ts... bytes) const
	{
		emit_byte(byte);
		emit_byte(bytes...);
	}

	template<typename T>
	typename std::enable_if_t<std::is_convertible_v<T, Value>, void>
		emit_constant(T&& value)
	{
		emit_byte(OpCode::Constant, make_constant(std::forward<T>(value)));
	}

	template<typename T>
	[[nodiscard]] typename std::enable_if_t<opcode_trait_v<T>, size_t>
		emit_jump(T&& instruction)const
	{
		emit_byte(std::forward<T>(instruction));
		emit_byte(static_cast<uint8_t>(0xff));
		emit_byte(static_cast<uint8_t>(0xff));
		return current_chunk().count() - 2;
	}
	void emit_loop(size_t loop_start);
	void emit_return()const;
	void patch_jump(size_t offset);

	[[nodiscard]] Chunk& current_chunk()const noexcept;

public:
	constexpr static ParseRule rules[40] = {
	  { &Compilation::grouping, &Compilation::call, Precedence::Call },       // TokenType::LEFT_PAREN      
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::RIGHT_PAREN     
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::LEFT_BRACE
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::RIGHT_BRACE     
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::COMMA           
	  { nullptr,     &Compilation::dot,    Precedence::Call },       // TokenType::DOT             
	  { &Compilation::unary, &Compilation::binary, Precedence::Term },       // TokenType::MINUS           
	  { nullptr,     &Compilation::binary, Precedence::Term },       // TokenType::PLUS            
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::SEMICOLON       
	  { nullptr,     &Compilation::binary,  Precedence::Factor },     // TokenType::SLASH           
	  { nullptr,     &Compilation::binary,  Precedence::Factor },     // TokenType::STAR            
	  { &Compilation::unary,     nullptr,   Precedence::None },       // TokenType::BANG            
	  { nullptr,     &Compilation::binary,  Precedence::Equality },       // TokenType::BANG_EQUAL      
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::EQUAL           
	  { nullptr,     &Compilation::binary,  Precedence::Equality },       // TokenType::EQUAL_EQUAL     
	  { nullptr,     &Compilation::binary,  Precedence::Comparison },       // TokenType::GREATER         
	  { nullptr,     &Compilation::binary,  Precedence::Comparison },       // TokenType::GREATER_EQUAL   
	  { nullptr,     &Compilation::binary,  Precedence::Comparison },       // TokenType::LESS            
	  { nullptr,     &Compilation::binary,  Precedence::Comparison },       // TokenType::LESS_EQUAL      
	  { &Compilation::variable, nullptr,    Precedence::None },       // TokenType::IDENTIFIER      
	  { &Compilation::string,   nullptr,    Precedence::None },       // TokenType::STRING          
	  { &Compilation::number,   nullptr,    Precedence::None },       // TokenType::NUMBER          
	  { nullptr,     &Compilation::and_,    Precedence::And },       // TokenType::AND             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::CLASS           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::ELSE            
	  { &Compilation::literal,  nullptr,    Precedence::None },       // TokenType::FALSE           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::FOR             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::FUN             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::IF              
	  { &Compilation::literal,  nullptr,    Precedence::None },       // TokenType::NIL             
	  { nullptr,     &Compilation::or_,    Precedence::Or },       // TokenType::OR              
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::PRINT           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::RETURN          
	  { &Compilation::super_,    nullptr,    Precedence::None },       // TokenType::SUPER           
	  { &Compilation::this_,     nullptr,    Precedence::None },       // TokenType::THIS            
	  { &Compilation::literal,   nullptr,    Precedence::None },       // TokenType::TRUE            
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::VAR             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::WHILE           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::ERROR           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::EOF             
	};

};

} //Clox