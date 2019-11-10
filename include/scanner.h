#pragma once

#include <string_view>

namespace Clox {

enum class TokenType :uint8_t
{
	// Single-character tokens.
	LeftParen, RightParen, LeftBrace, RightBrace,
	Comma, Dot, Minus, Plus,
	Semicolon, Slash, Star,

	// One or two character tokens.
	Bang, BangEqual, Equal, EqualEqual,
	Greater, GreaterEqual, Less, LessEqual,

	// Literals.
	Identifier, String, Number,

	// Keywords.
	And, Class, Else, False,
	Fun, For, If, Nil,
	Or, Print, Return, Super,
	This, True, Var, While,

	Error, Eof,
};

struct Token
{
	TokenType type;
	std::string_view text;
	size_t line = 1;

	constexpr Token(TokenType type, std::string_view text, size_t line)noexcept
		:type(type), text(text), line(line)
	{
	}
	Token() = default;
};

struct Scanner
{
	size_t start = 0;
	size_t current = 0;
	size_t line = 1;
	std::string_view source;

	constexpr explicit Scanner(std::string_view source) noexcept
		:source(source)
	{
	}

	Token scan_token();

private:
	Token identifier();
	Token number();
	Token string();
	void skip_whitespace();
	TokenType identifier_type()const;
	TokenType check_keyword(size_t begin, size_t length, std::string_view rest, TokenType type)const;

	Token error_token(std::string_view message)const noexcept;
	Token make_token(TokenType type)const noexcept;

	char advance();
	bool is_at_end()const noexcept;
	bool match(char expected);
	char peek()const;
	char peek_next()const;
};

} //Clox