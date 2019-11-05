#pragma once

#include <array>
#include <optional>

#include "chunk.h"
#include "scanner.h"

namespace Clox {

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

struct Parser
{
	Token current;
	Token previous;
	bool had_error = false;
	bool panic_mode = false;
};

struct Compilation;
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
	Script,
};

struct Local
{
	Token name;
	int depth = -1;
};

struct Compiler
{
	ObjFunction* function = nullptr;
	FunctionType type = FunctionType::Script;

	std::array<Local, UINT8_COUNT> locals = {};
	size_t local_count = 0;
	int scope_depth = 0;
};

struct VM;

struct Compilation
{
	std::unique_ptr<Compiler> current = nullptr;
	Parser parser;
	Scanner scanner;
	VM& vm;

	static ObjFunction* compile(std::string_view source, VM& vm);

	Compilation(std::string_view source, VM& vm)
		:parser(), scanner(source), vm(vm)
	{
	}

private:

	void expression();
	void and_([[maybe_unused]] bool can_assign);
	void binary([[maybe_unused]] bool can_assign);
	void grouping([[maybe_unused]] bool can_assign);
	void literal([[maybe_unused]] bool can_assign);
	void number([[maybe_unused]] bool can_assign);
	void or_([[maybe_unused]] bool can_assign);
	void string([[maybe_unused]] bool can_assign);
	void unary([[maybe_unused]] bool can_assign);
	void variable(bool can_assign);

	void statement();
	void block();
	void expression_statement();
	void for_statement();
	void if_statement();
	void print_statement();
	void while_statement();

	void declaration();
	void var_declaration();

	void declare_variable();
	void define_variable(uint8_t global)const;
	uint8_t identifier_constant(const Token& name);
	void named_variable(const Token& name, bool can_assign);
	void parse_precedence(Precedence precedence);
	uint8_t parse_variable(std::string_view error);

	void init_compiler(FunctionType type);
	void add_local(const Token& name);
	void begin_scope()const;
	void end_scope()const;
	void mark_initializied()const;
	std::optional<uint8_t> resolve_local(const Token& name);

	template<typename T>
	typename std::enable_if_t<std::is_convertible_v<T, Value>, uint8_t>
		make_constant(T&& value)
	{
		auto constant = current_chunk().add_constant(std::forward<T>(value));
		if (constant > UINT8_MAX)
		{
			error("Too many constants in one chunk.");
			return 0;
		}
		return static_cast<uint8_t>(constant);
	}

	template<typename T>
	typename std::enable_if_t<opcode_trait<T>, void>
		emit_byte(T byte) const
	{
		current_chunk().write(byte, parser.previous.line);
	}

	template<typename T1, typename T2>
	typename std::enable_if_t<opcode_trait<T1> && opcode_trait<T2>, void>
		emit_bytes(T1 b1, T2 b2) const
	{
		emit_byte(b1);
		emit_byte(b2);
	}

	template<typename T>
	typename std::enable_if_t<std::is_convertible_v<T, Value>, void>
		emit_constant(T&& value)
	{
		emit_bytes(OpCode::Constant, make_constant(std::forward<T>(value)));
	}

	template<typename T>
	typename std::enable_if_t<opcode_trait<T>, size_t>
		emit_jump(T&& instruction)
	{
		emit_byte(std::forward<T>(instruction));
		emit_byte(static_cast<uint8_t>(0xff));
		emit_byte(static_cast<uint8_t>(0xff));
		return current_chunk().count() - 2;
	}
	void emit_loop(size_t loop_start);
	void emit_return()const { emit_byte(OpCode::Return); }
	void patch_jump(size_t offset);

	void advance();
	bool check(TokenType type)const noexcept;
	void consume(TokenType type, std::string_view message);
	bool match(TokenType type);
	ObjFunction* end_compiler()const;

	Chunk& current_chunk()const noexcept;

	void error(std::string_view message);
	void error_at_current(std::string_view message);
	void error_at(const Token& token, std::string_view message);
	void synchronize();

public:
	constexpr static ParseRule rules[40] = {
	  { &Compilation::grouping, nullptr,    Precedence::None },       // TokenType::LEFT_PAREN      
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::RIGHT_PAREN     
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::LEFT_BRACE
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::RIGHT_BRACE     
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::COMMA           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::DOT             
	  { &Compilation::unary,    &Compilation::binary,  Precedence::Term },       // TokenType::MINUS           
	  { nullptr,     &Compilation::binary,  Precedence::Term },       // TokenType::PLUS            
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::SEMICOLON       
	  { nullptr,     &Compilation::binary,  Precedence::Factor },     // TokenType::SLASH           
	  { nullptr,     &Compilation::binary,  Precedence::Factor },     // TokenType::STAR            
	  { &Compilation::unary,     nullptr,    Precedence::None },       // TokenType::BANG            
	  { nullptr,     &Compilation::binary,    Precedence::Equality },       // TokenType::BANG_EQUAL      
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::EQUAL           
	  { nullptr,     &Compilation::binary,    Precedence::Equality },       // TokenType::EQUAL_EQUAL     
	  { nullptr,     &Compilation::binary,    Precedence::Comparison },       // TokenType::GREATER         
	  { nullptr,     &Compilation::binary,    Precedence::Comparison },       // TokenType::GREATER_EQUAL   
	  { nullptr,     &Compilation::binary,    Precedence::Comparison },       // TokenType::LESS            
	  { nullptr,     &Compilation::binary,    Precedence::Comparison },       // TokenType::LESS_EQUAL      
	  { &Compilation::variable,     nullptr,    Precedence::None },       // TokenType::IDENTIFIER      
	  { &Compilation::string,     nullptr,    Precedence::None },       // TokenType::STRING          
	  { &Compilation::number,   nullptr,    Precedence::None },       // TokenType::NUMBER          
	  { nullptr,     &Compilation::and_,    Precedence::And },       // TokenType::AND             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::CLASS           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::ELSE            
	  { &Compilation::literal,     nullptr,    Precedence::None },       // TokenType::FALSE           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::FOR             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::FUN             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::IF              
	  { &Compilation::literal,     nullptr,    Precedence::None },       // TokenType::NIL             
	  { nullptr,     &Compilation::or_,    Precedence::Or },       // TokenType::OR              
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::PRINT           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::RETURN          
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::SUPER           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::THIS            
	  { &Compilation::literal,     nullptr,    Precedence::None },       // TokenType::TRUE            
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::VAR             
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::WHILE           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::ERROR           
	  { nullptr,     nullptr,    Precedence::None },       // TokenType::EOF             
	};

};

} //Clox