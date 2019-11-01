#include "compiler.h"

#include <cstdlib>
#include <iostream>

#include "debug.h"
#include "object_t.h"

namespace Clox {

const ParseRule& get_rule(TokenType type) noexcept
{
	return Compilation::rules[static_cast<size_t>(type)];
}

bool Compilation::compile(std::string_view source, VM& vm)
{
	Compilation cu(source, vm);
	cu.init_compiler();
	cu.advance();
	while (!cu.match(TokenType::Eof))
		cu.declaration();

	cu.end_compiler();
	return !cu.parser.had_error;
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
	auto op = parser.previous.type;
	const auto& rule = get_rule(op);
	parse_precedence(rule.precedence + 1);

	switch (op)
	{
		case TokenType::BangEqual: emit_bytes(OpCode::Equal, OpCode::Not); break;
		case TokenType::EqualEqual: emit_byte(OpCode::Equal); break;
		case TokenType::Greater: emit_byte(OpCode::Greater); break;
		case TokenType::GreaterEqual: emit_bytes(OpCode::Less, OpCode::Not); break;
		case TokenType::Less: emit_byte(OpCode::Less); break;
		case TokenType::LessEqual: emit_bytes(OpCode::Greater, OpCode::Not); break;
		case TokenType::Plus: emit_byte(OpCode::Add); break;
		case TokenType::Minus: emit_byte(OpCode::Subtract); break;
		case TokenType::Star: emit_byte(OpCode::Multiply); break;
		case TokenType::Slash: emit_byte(OpCode::Divide); break;
		default:
			break;
	}
}

void Compilation::grouping([[maybe_unused]] bool can_assign)
{
	expression();
	consume(TokenType::RightParen, "Expect ')' after expression.");
}

void Compilation::literal([[maybe_unused]] bool can_assign)
{
	switch (parser.previous.type)
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
	auto value = std::strtod(parser.previous.text.data(), nullptr);
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
	const auto& text = parser.previous.text;
	auto str = text.substr(1, text.size() - 2);
	emit_constant(create_obj_string(str, vm));
}

void Compilation::unary([[maybe_unused]] bool can_assign)
{
	auto op = parser.previous.type;
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
	named_variable(parser.previous, can_assign);
}

void Compilation::statement()
{
	if (match(TokenType::For))
		for_statement();
	else if (match(TokenType::If))
		if_statement();
	else if (match(TokenType::Print))
		print_statement();
	else if (match(TokenType::While))
		while_statement();
	else if (match(TokenType::LeftBrace))
	{
		begin_scope();
		block();
		end_scope();
	} else
		expression_statement();
}

void Compilation::block()
{
	while (!check(TokenType::RightBrace) && !check(TokenType::Eof))
		declaration();
	consume(TokenType::RightBrace, "Expect '}' after block.");
}

void Compilation::expression_statement()
{
	expression();
	consume(TokenType::Semicolon, "Expect ';' after expression.");
	emit_byte(OpCode::Pop);
}

void Compilation::for_statement()
{
	begin_scope();

	consume(TokenType::LeftParen, "Expect '(' after 'for'.");
	if (match(TokenType::Semicolon))
	{
	} else if (match(TokenType::Var))
		var_declaration();
	else expression_statement();

	auto loop_start = current_chunk().count();

	std::optional<size_t> exit_jump = std::nullopt;
	if (!match(TokenType::Semicolon))
	{
		expression();
		consume(TokenType::Semicolon, "Expect ';' after loop condition.");

		exit_jump = emit_jump(OpCode::JumpIfFalse);
		emit_byte(OpCode::Pop);
	}

	if (!match(TokenType::RightParen))
	{
		auto body_jump = emit_jump(OpCode::Jump);

		auto increment_start = current_chunk().count();
		expression();
		emit_byte(OpCode::Pop);
		consume(TokenType::RightParen, "Expect ')' after for clauses.");

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
	consume(TokenType::LeftParen, "Expect '(' after 'if'.");
	expression();
	consume(TokenType::RightParen, "Expect ')' after condition.");

	auto then_jump = emit_jump(OpCode::JumpIfFalse);
	emit_byte(OpCode::Pop);
	statement();

	auto else_jump = emit_jump(OpCode::Jump);

	patch_jump(then_jump);

	if (match(TokenType::Else))
		statement();
	patch_jump(else_jump);
	emit_byte(OpCode::Pop);
}

void Compilation::while_statement()
{
	auto loop_start = current_chunk().count();
	consume(TokenType::LeftParen, "Expect '(' after 'while'.");
	expression();
	consume(TokenType::RightParen, "Expect ')' after condition.");

	auto exit_jump = emit_jump(OpCode::JumpIfFalse);

	emit_byte(OpCode::Pop);
	statement();

	emit_loop(loop_start);

	patch_jump(exit_jump);
	emit_byte(OpCode::Pop);
}

void Compilation::print_statement()
{
	expression();
	consume(TokenType::Semicolon, "Expect ';' after value.");
	emit_byte(OpCode::Print);
}

void Compilation::declaration()
{
	if (match(TokenType::Var))
		var_declaration();
	else
		statement();

	if (parser.panic_mode)
		synchronize();
}

void Compilation::var_declaration()
{
	auto global = parse_variable("Expect variable name.");
	if (match(TokenType::Equal))
		expression();
	else
		emit_byte(OpCode::Nil);

	consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
	define_variable(global);
}

void Compilation::declare_variable()
{
	if (current->scope_depth == 0) return;

	const auto& name = parser.previous;

	for (int i = current->local_count - 1; i >= 0; i--)
	{
		const auto& local = current->locals.at(i);
		if (local.depth != -1 && local.depth < current->scope_depth)
			break;
		if (name.text == local.name.text)
			error("Variable with this name already declared in this scope.");
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

	emit_bytes(OpCode::DefineGlobal, global);
}

uint8_t Compilation::identifier_constant(const Token& name)
{
	return make_constant(create_obj_string(name.text, vm));
}

void Compilation::named_variable(const Token& name, bool can_assign)
{
	OpCode get_op, set_op;
	auto arg = resolve_local(name);
	if (arg.has_value())
	{
		get_op = OpCode::GetLocal;
		set_op = OpCode::SetLocal;
	} else
	{
		arg = identifier_constant(name);
		get_op = OpCode::GetGlobal;
		set_op = OpCode::SetGlobal;
	}
	if (can_assign && match(TokenType::Equal))
	{
		expression();
		emit_bytes(set_op, arg.value());
	} else
	{
		emit_bytes(get_op, arg.value());
	}
}

void Compilation::parse_precedence(Precedence precedence)
{
	advance();

	auto prefix_rule = get_rule(parser.previous.type).prefix;
	if (prefix_rule == nullptr)
	{
		error("Expect expression.");
		return;
	}

	bool can_assign = precedence <= Precedence::Assignment;
	(this->*prefix_rule)(can_assign);

	while (precedence <= get_rule(parser.current.type).precedence)
	{
		advance();
		auto infix_rule = get_rule(parser.previous.type).infix;
		(this->*infix_rule)(can_assign);

		if (can_assign && match(TokenType::Equal))
		{
			error("Invalid assignment target.");
			expression();
		}
	}
}

uint8_t Compilation::parse_variable(std::string_view error)
{
	consume(TokenType::Identifier, error);

	declare_variable();
	if (current->scope_depth > 0) return 0;

	return identifier_constant(parser.previous);
}

void Compilation::init_compiler()
{
	current = std::make_unique<Compiler>();
}

void Compilation::add_local(const Token& name)
{
	if (current->local_count == UINT8_COUNT)
	{
		error("Too many local variables in function.");
		return;
	}
	auto& local = current->locals.at(current->local_count++);
	local.name = name;
	local.depth = -1;
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
		emit_byte(OpCode::Pop);
		current->local_count--;
	}
}

void Compilation::mark_initializied() const
{
	current->locals.at(current->local_count - 1).depth = current->scope_depth;
}

std::optional<uint8_t> Compilation::resolve_local(const Token& name)
{
	for (int i = current->local_count - 1; i >= 0; i--)
	{
		if (i > UINT8_COUNT)
			i = UINT8_COUNT - 1;
		const auto& local = current->locals.at(i);
		if (name.text == local.name.text)
		{
			if (local.depth == -1)
				error("Cannot read local variable in its own initializer.");
			return i;
		}
	}
	return std::nullopt;
}

void Compilation::emit_loop(size_t loop_start)
{
	emit_byte(OpCode::Loop);

	auto offset = current_chunk().count() - loop_start + 2;
	if (offset > UINT16_MAX)
		error("Loop body too large.");
	emit_byte(static_cast<uint8_t>((offset >> 8) & 0xff));
	emit_byte(static_cast<uint8_t>(offset & 0xff));
}

void Compilation::patch_jump(size_t offset)
{
	auto jump = current_chunk().count() - offset - 2;

	if (jump > UINT16_MAX)
		error("Too much code to jump over.");

	current_chunk().code.at(offset) = static_cast<OpCode>((jump >> 8) & 0xff);
	current_chunk().code.at(offset + 1) = static_cast<OpCode>(jump & 0xff);
}

void Compilation::advance()
{
	parser.previous = parser.current;
	while (true)
	{
		parser.current = scanner.scan_token();
		if (parser.current.type != TokenType::Error)
			break;
		error_at_current(parser.current.text);
	}
}

bool Compilation::check(TokenType type) const noexcept
{
	return parser.current.type == type;
}

void Compilation::consume(TokenType type, std::string_view message)
{
	if (parser.current.type == type)
	{
		advance();
		return;
	}
	error_at_current(message);
}

bool Compilation::match(TokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
}

void Compilation::end_compiler() const
{
	emit_return();
	//#ifdef _DEBUG
	//	if (!parser.had_error)
	//		disassemble_chunk(current_chunk(), "code");
	//#endif // _DEBUG
}

Chunk& Compilation::current_chunk() const noexcept
{
	return vm.chunk;
}

void Compilation::error(std::string_view message)
{
	error_at(parser.previous, message);
}

void Compilation::error_at_current(std::string_view message)
{
	error_at(parser.current, message);
}

void Compilation::error_at(const Token& token, std::string_view message)
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

void Compilation::synchronize()
{
	parser.panic_mode = false;

	while (parser.current.type != TokenType::Eof)
	{
		if (parser.previous.type == TokenType::Semicolon)
			return;

		switch (parser.current.type)
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

} //Clox