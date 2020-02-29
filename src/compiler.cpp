#include "compiler.h"

#include <cstdlib>
#include <iostream>

#include "obj_string.h"
#include "vm.h"

#ifdef _DEBUG
//#define DEBUG_PRINT_CODE
#endif // _DEBUG

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif // DEBUG_PRINT_CODE

namespace Clox {

[[nodiscard]] const ParseRule& get_rule(TokenType type) noexcept
{
	return Compilation::rules[static_cast<size_t>(type)];
}

void error(Parser& parser, std::string_view message)
{
	error_at(parser, parser.previous, message);
}

void error_at_current(Parser& parser, std::string_view message)
{
	error_at(parser, parser.current, message);
}

void error_at(Parser& parser, const Token& token, std::string_view message)
{
	if (parser.panic_mode) return;
	parser.panic_mode = true;
	std::cerr << "[line " << token.line << "] Error";
	if (token.type == TokenType::Eof)
		std::cerr << " at end";
	else if (token.type == TokenType::Error)
	{
	} else
		std::cerr << " at " << token.text;
	std::cerr << ": " << message << '\n';
	parser.had_error = true;
}

void Parser::advance()
{
	previous = current;
	while (true)
	{
		current = scanner.scan_token();
		if (current.type != TokenType::Error)
			break;
		error_at_current(*this, current.text);
	}
}

bool Parser::check(TokenType type) const noexcept
{
	return current.type == type;
}

void Parser::consume(TokenType type, std::string_view message)
{
	if (current.type == type)
	{
		advance();
		return;
	}
	error_at_current(*this, message);
}

bool Parser::match(TokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
}

void Parser::synchronize()
{
	panic_mode = false;

	while (current.type != TokenType::Eof)
	{
		if (previous.type == TokenType::Semicolon)
			return;

		switch (current.type)
		{
			case TokenType::Class:
			case TokenType::Fun:
			case TokenType::Var:
			case TokenType::For:
			case TokenType::If:
			case TokenType::While:
			case TokenType::Print:
			case TokenType::Return:
				return;
			default:
				break;
		}
		advance();
	}
}

ObjFunction* Compilation::compile(std::string_view source)
{
	parser = std::make_unique<Parser>(source);
	init_compiler(FunctionType::Script);
	parser->advance();
	while (!parser->match(TokenType::Eof))
		declaration();

	auto [function, done] = end_compiler();
	return parser->had_error ? nullptr : function;
}

void Compilation::expression()
{
	parse_precedence(Precedence::Assignment);
}

void Compilation::and_([[maybe_unused]] bool can_assign)
{
	auto end_jump = emit_jump(OpCode::JumpIfFalse);
	emit_byte(OpCode::Pop);
	parse_precedence(Precedence::And);
	patch_jump(end_jump);
}

void Compilation::binary([[maybe_unused]] bool can_assign)
{
	auto op = parser->previous.type;
	const auto& rule = get_rule(op);
	parse_precedence(rule.precedence + 1);

	switch (op)
	{
		case TokenType::BangEqual: emit_byte(OpCode::Equal, OpCode::Not); break;
		case TokenType::EqualEqual: emit_byte(OpCode::Equal); break;
		case TokenType::Greater: emit_byte(OpCode::Greater); break;
		case TokenType::GreaterEqual: emit_byte(OpCode::Less, OpCode::Not); break;
		case TokenType::Less: emit_byte(OpCode::Less); break;
		case TokenType::LessEqual: emit_byte(OpCode::Greater, OpCode::Not); break;
		case TokenType::Plus: emit_byte(OpCode::Add); break;
		case TokenType::Minus: emit_byte(OpCode::Subtract); break;
		case TokenType::Star: emit_byte(OpCode::Multiply); break;
		case TokenType::Slash: emit_byte(OpCode::Divide); break;
		default:
			break;
	}
}

void Compilation::call([[maybe_unused]] bool can_assign)
{
	auto arg_count = argument_list();
	emit_byte(OpCode::Call, arg_count);
}

void Compilation::dot(bool can_assign)
{
	parser->consume(TokenType::Identifier, "Expect property name after '.'.");
	auto name = identifier_constant(parser->previous);

	if (can_assign && parser->match(TokenType::Equal))
	{
		expression();
		emit_byte(OpCode::SetProperty, name);
	} else if (parser->match(TokenType::LeftParen))
	{
		auto arg_count = argument_list();
		emit_byte(OpCode::Invoke, name, arg_count);
	}
	else
	{
		emit_byte(OpCode::GetProperty, name);
	}
}

void Compilation::grouping([[maybe_unused]] bool can_assign)
{
	expression();
	parser->consume(TokenType::RightParen, "Expect ')' after expression.");
}

void Compilation::literal([[maybe_unused]] bool can_assign)
{
	switch (parser->previous.type)
	{
		case TokenType::False: emit_byte(OpCode::False); break;
		case TokenType::Nil: emit_byte(OpCode::Nil); break;
		case TokenType::True: emit_byte(OpCode::True); break;
		default:
			break;
	}
}

void Compilation::number([[maybe_unused]] bool can_assign)
{
	auto value = std::strtod(parser->previous.text.data(), nullptr);
	emit_constant(value);
}

void Compilation::or_([[maybe_unused]] bool can_assign)
{
	auto else_jump = emit_jump(OpCode::JumpIfFalse);
	auto end_jump = emit_jump(OpCode::Jump);

	patch_jump(else_jump);
	emit_byte(OpCode::Pop);

	parse_precedence(Precedence::Or);
	patch_jump(end_jump);
}

void Compilation::string([[maybe_unused]] bool can_assign)
{
	const auto& text = parser->previous.text;
	auto str = text.substr(1, text.size() - 2);
	emit_constant(create_obj_string(str, vm));
}

void Compilation::unary([[maybe_unused]] bool can_assign)
{
	auto op = parser->previous.type;
	parse_precedence(Precedence::Unary);
	switch (op)
	{
		case TokenType::Bang: emit_byte(OpCode::Not); break;
		case TokenType::Minus: emit_byte(OpCode::Negate); break;
		default:
			break;
	}
}

void Compilation::variable(bool can_assign)
{
	named_variable(parser->previous, can_assign);
}

void Compilation::this_([[maybe_unused]] bool can_assign)
{
	if (current_class == nullptr)
	{
		error(*parser, "Cannot use 'this' outside of a class");
		return;
	}
	variable(false);
}

void Compilation::statement()
{
	if (parser->match(TokenType::For))
		for_statement();
	else if (parser->match(TokenType::If))
		if_statement();
	else if (parser->match(TokenType::Print))
		print_statement();
	else if (parser->match(TokenType::Return))
		return_statement();
	else if (parser->match(TokenType::While))
		while_statement();
	else if (parser->match(TokenType::LeftBrace))
	{
		begin_scope();
		block();
		end_scope();
	} else
		expression_statement();
}

void Compilation::block()
{
	while (!parser->check(TokenType::RightBrace) && !parser->check(TokenType::Eof))
		declaration();
	parser->consume(TokenType::RightBrace, "Expect '}' after block.");
}

void Compilation::expression_statement()
{
	expression();
	parser->consume(TokenType::Semicolon, "Expect ';' after expression.");
	emit_byte(OpCode::Pop);
}

void Compilation::for_statement()
{
	begin_scope();

	parser->consume(TokenType::LeftParen, "Expect '(' after 'for'.");
	if (parser->match(TokenType::Semicolon))
	{
	} else if (parser->match(TokenType::Var))
		var_declaration();
	else expression_statement();

	auto loop_start = current_chunk().count();

	std::optional<size_t> exit_jump = std::nullopt;
	if (!parser->match(TokenType::Semicolon))
	{
		expression();
		parser->consume(TokenType::Semicolon, "Expect ';' after loop condition.");

		exit_jump = emit_jump(OpCode::JumpIfFalse);
		emit_byte(OpCode::Pop);
	}

	if (!parser->match(TokenType::RightParen))
	{
		auto body_jump = emit_jump(OpCode::Jump);

		auto increment_start = current_chunk().count();
		expression();
		emit_byte(OpCode::Pop);
		parser->consume(TokenType::RightParen, "Expect ')' after for clauses.");

		emit_loop(loop_start);
		loop_start = increment_start;
		patch_jump(body_jump);
	}

	statement();

	emit_loop(loop_start);
	if (exit_jump.has_value())
	{
		patch_jump(exit_jump.value());
		emit_byte(OpCode::Pop);
	}

	end_scope();
}

void Compilation::if_statement()
{
	parser->consume(TokenType::LeftParen, "Expect '(' after 'if'.");
	expression();
	parser->consume(TokenType::RightParen, "Expect ')' after condition.");

	auto then_jump = emit_jump(OpCode::JumpIfFalse);
	emit_byte(OpCode::Pop);
	statement();

	auto else_jump = emit_jump(OpCode::Jump);

	patch_jump(then_jump);

	if (parser->match(TokenType::Else))
		statement();
	patch_jump(else_jump);
	emit_byte(OpCode::Pop);
}

void Compilation::print_statement()
{
	expression();
	parser->consume(TokenType::Semicolon, "Expect ';' after value.");
	emit_byte(OpCode::Print);
}

void Compilation::return_statement()
{
	if (current->type == FunctionType::Script)
		error(*parser, "Cannot return from top-level code.");

	if (parser->match(TokenType::Semicolon))
		emit_return();
	else
	{
		if (current->type == FunctionType::Initializer)
			error(*parser, "Cannot return a value from an initializer");
		expression();
		parser->consume(TokenType::Semicolon, "Expect ';' after return value.");
		emit_byte(OpCode::Return);
	}
}

void Compilation::while_statement()
{
	auto loop_start = current_chunk().count();
	parser->consume(TokenType::LeftParen, "Expect '(' after 'while'.");
	expression();
	parser->consume(TokenType::RightParen, "Expect ')' after condition.");

	auto exit_jump = emit_jump(OpCode::JumpIfFalse);

	emit_byte(OpCode::Pop);
	statement();

	emit_loop(loop_start);

	patch_jump(exit_jump);
	emit_byte(OpCode::Pop);
}

void Compilation::declaration()
{
	if (parser->match(TokenType::Class))
		class_declaration();
	else if (parser->match(TokenType::Fun))
		fun_declaration();
	else if (parser->match(TokenType::Var))
		var_declaration();
	else
		statement();

	if (parser->panic_mode)
		parser->synchronize();
}

void Compilation::class_declaration()
{
	parser->consume(TokenType::Identifier, "Expect class name.");
	auto& class_name = parser->previous;
	auto name_constant = identifier_constant(parser->previous);
	declare_variable();

	emit_byte(OpCode::Class, name_constant);
	define_variable(name_constant);

	auto class_compiler = std::make_unique<ClassCompiler>();
	class_compiler->name = parser->previous;
	class_compiler->enclosing = std::move(current_class);
	current_class = std::move(class_compiler);

	named_variable(class_name, false);
	parser->consume(TokenType::LeftBrace, "Expect '{' before class body.");
	while (!parser->check(TokenType::RightBrace) && !parser->check(TokenType::Eof))
		method();
	parser->consume(TokenType::RightBrace, "Expect '}' after class body.");
	emit_byte(OpCode::Pop);

	current_class = std::move(current_class->enclosing);
}

void Compilation::fun_declaration()
{
	auto global = parse_variable("Expect function name.");
	mark_initializied();
	function(FunctionType::Function);
	define_variable(global);
}

void Compilation::var_declaration()
{
	auto global = parse_variable("Expect variable name.");
	if (parser->match(TokenType::Equal))
		expression();
	else
		emit_byte(OpCode::Nil);

	parser->consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
	define_variable(global);
}

void Compilation::function(FunctionType type)
{
	init_compiler(type);
	begin_scope();

	parser->consume(TokenType::LeftParen, "Expect '(' after function name.");
	if (!parser->check(TokenType::RightParen))
	{
		do
		{
			current->function->arity++;
			if (current->function->arity > 255)
				error_at_current(*parser, "Cannot have more than 255 parameters.");
			auto param_count = parse_variable("Expect parameter name.");
			define_variable(param_count);
		} while (parser->match(TokenType::Comma));
	}
	parser->consume(TokenType::RightParen, "Expect ')' after parameters.");

	parser->consume(TokenType::LeftBrace, "Expect '{' before function body.");
	block();

	auto [function, done] = end_compiler();
	emit_byte(OpCode::Closure, make_constant(function));

	for (size_t i = 0; i < function->upvalue_count; i++)
	{
		emit_byte(static_cast<uint8_t>(done->upvalues.at(i).is_local ? 1 : 0));
		emit_byte(done->upvalues.at(i).index);
	}
}

void Compilation::method()
{
	parser->consume(TokenType::Identifier, "Expect method name.");
	auto constant = identifier_constant(parser->previous);
	auto type = FunctionType::Method;
	if (parser->previous.text == "init")
		type = FunctionType::Initializer;
	function(type);
	emit_byte(OpCode::Method, constant);
}

uint8_t Compilation::argument_list()
{
	uint8_t arg_count = 0;
	if (!parser->check(TokenType::RightParen))
	{
		do
		{
			expression();
			if (arg_count == 255)
				error(*parser, "Cannot have more than 255 arguments.");
			arg_count++;
		} while (parser->match(TokenType::Comma));
	}
	parser->consume(TokenType::RightParen, "Expect ')' after arguments.");
	return arg_count;
}

void Compilation::declare_variable()
{
	if (current->scope_depth == 0) return;

	const auto& name = parser->previous;

	for (int i = current->local_count - 1; i >= 0; i--)
	{
		const auto& local = current->locals.at(i);
		if (local.depth != -1 && local.depth < current->scope_depth)
			break;
		if (name.text == local.name.text)
			error(*parser, "Variable with this name already declared in this scope.");
	}

	add_local(name);
}

void Compilation::define_variable(uint8_t global) const
{
	if (current->scope_depth > 0)
	{
		mark_initializied();
		return;
	}

	emit_byte(OpCode::DefineGlobal, global);
}

uint8_t Compilation::identifier_constant(const Token& name)
{
	return make_constant(create_obj_string(name.text, vm));
}

void Compilation::named_variable(const Token& name, bool can_assign)
{
	OpCode get_op, set_op;
	auto arg = resolve_local(current, name);
	if (arg.has_value())
	{
		get_op = OpCode::GetLocal;
		set_op = OpCode::SetLocal;
	} else
	{
		arg = resolve_upvalue(current, name);
		if (arg.has_value())
		{
			get_op = OpCode::GetUpvalue;
			set_op = OpCode::SetUpvalue;
		} else
		{
			arg = identifier_constant(name);
			get_op = OpCode::GetGlobal;
			set_op = OpCode::SetGlobal;
		}
	}
	if (can_assign && parser->match(TokenType::Equal))
	{
		expression();
		emit_byte(set_op, arg.value());
	} else
	{
		emit_byte(get_op, arg.value());
	}
}

void Compilation::parse_precedence(Precedence precedence)
{
	parser->advance();

	auto prefix_rule = get_rule(parser->previous.type).prefix;
	if (prefix_rule == nullptr)
	{
		error(*parser, "Expect expression.");
		return;
	}

	bool can_assign = precedence <= Precedence::Assignment;
	(this->*prefix_rule)(can_assign);

	while (precedence <= get_rule(parser->current.type).precedence)
	{
		parser->advance();
		auto infix_rule = get_rule(parser->previous.type).infix;
		(this->*infix_rule)(can_assign);

		if (can_assign && parser->match(TokenType::Equal))
		{
			error(*parser, "Invalid assignment target.");
			expression();
		}
	}
}

uint8_t Compilation::parse_variable(std::string_view error)
{
	parser->consume(TokenType::Identifier, error);

	declare_variable();
	if (current->scope_depth > 0) return 0;

	return identifier_constant(parser->previous);
}

void Compilation::init_compiler(FunctionType type)
{
	auto compiler = std::make_unique<Compiler>();
	compiler->enclosing = std::move(current);
	current = std::move(compiler);
	current->function = create_obj<ObjFunction>(vm.gc);
	current->type = type;

	if (type != FunctionType::Script)
		current->function->name = create_obj_string(parser->previous.text, vm);

	auto& local = current->locals.at(current->local_count++);
	local.depth = 0;
	local.is_captured = false;
	if (type != FunctionType::Function)
		local.name.text = "this";
	else
		local.name.text = std::string_view();
}

auto Compilation::end_compiler()->std::pair<ObjFunction*, std::unique_ptr<Compiler>>
{
	emit_return();
	auto function = current->function;

#ifdef DEBUG_PRINT_CODE
	if (!parser->had_error)
		disassemble_chunk(current_chunk(),
			function->name == nullptr ? "<script>" : function->name->text());
#endif // DEBUG_PRINT_CODE

	// return ended compiler out to Compilation::function
	// so that it has access to array<upvalue>
	std::unique_ptr<Compiler> done = std::move(current);
	current = std::move(done->enclosing);
	return std::make_pair(function, std::move(done));
}

void Compilation::add_local(const Token& name)
{
	if (current->local_count == UINT8_COUNT)
	{
		error(*parser, "Too many local variables in function.");
		return;
	}
	auto& local = current->locals.at(current->local_count++);
	local.name = name;
	local.depth = -1;
	local.is_captured = false;
}

uint8_t Compilation::add_upvalue(const std::unique_ptr<Compiler>& compiler, uint8_t index, bool is_local)
{
	auto upvalue_count = compiler->function->upvalue_count;
	for (size_t i = 0; i < upvalue_count; i++)
	{
		const auto& upvalue = compiler->upvalues.at(i);
		if (upvalue.index == index && upvalue.is_local == is_local)
			return i;
	}
	if (upvalue_count == UINT8_COUNT)
	{
		error(*parser, "Too many closure variables in function.");
		return 0;
	}
	compiler->upvalues.at(upvalue_count).is_local = is_local;
	compiler->upvalues.at(upvalue_count).index = index;
	return compiler->function->upvalue_count++;
}

void Compilation::begin_scope() const
{
	current->scope_depth++;
}

void Compilation::end_scope() const
{
	current->scope_depth--;

	while (current->local_count > 0 &&
		current->locals.at(current->local_count - 1).depth > current->scope_depth)
	{
		if (current->locals.at(current->local_count - 1).is_captured)
			emit_byte(OpCode::CloseUpvalue);
		else
			emit_byte(OpCode::Pop);
		current->local_count--;
	}
}

void Compilation::mark_initializied() const
{
	if (current->scope_depth == 0) return;

	current->locals.at(current->local_count - 1).depth = current->scope_depth;
}

std::optional<uint8_t> Compilation::resolve_local(const std::unique_ptr<Compiler>& compiler, const Token& name)
{
	for (int i = compiler->local_count - 1; i >= 0; i--)
	{
		const auto& local = compiler->locals.at(i);
		if (name.text == local.name.text)
		{
			if (local.depth == -1)
				error(*parser, "Cannot read local variable in its own initializer.");
			return i;
		}
	}
	return std::nullopt;
}

std::optional<uint8_t> Compilation::resolve_upvalue(const std::unique_ptr<Compiler>& compiler, const Token& name)
{
	if (compiler->enclosing == nullptr) return std::nullopt;

	auto local = resolve_local(compiler->enclosing, name);
	if (local.has_value())
	{
		compiler->enclosing->locals.at(local.value()).is_captured = true;
		return add_upvalue(compiler, local.value(), true);
	}

	auto upvalue = resolve_upvalue(compiler->enclosing, name);
	if (upvalue.has_value())
		return add_upvalue(compiler, upvalue.value(), false);

	return std::nullopt;
}

void Compilation::emit_loop(size_t loop_start)
{
	emit_byte(OpCode::Loop);

	auto offset = current_chunk().count() - loop_start + 2;
	if (offset > UINT16_MAX)
		error(*parser, "Loop body too large.");
	emit_byte(static_cast<uint8_t>((offset >> 8) & 0xff));
	emit_byte(static_cast<uint8_t>(offset & 0xff));
}

void Compilation::emit_return() const
{
	if (current->type == FunctionType::Initializer)
		emit_byte(OpCode::GetLocal, static_cast<uint8_t>(0));
	else
		emit_byte(OpCode::Nil);
	emit_byte(OpCode::Return);
}

void Compilation::patch_jump(size_t offset)
{
	auto jump = current_chunk().count() - offset - 2;

	if (jump > UINT16_MAX)
		error(*parser, "Too much code to jump over.");

	current_chunk().code.at(offset) = static_cast<uint8_t>((jump >> 8) & 0xff);
	current_chunk().code.at(offset + 1) = static_cast<uint8_t>(jump & 0xff);
}

Chunk& Compilation::current_chunk() const noexcept
{
	return current->function->chunk;
}

} //Clox