#include "scanner.h"

#include <cctype>
#include <iostream>

namespace Clox {

Token Scanner::scan_token()
{
	skip_whitespace();

	start = current;
	if (is_at_end()) return make_token(TokenType::Eof);

	auto c = advance();
	if (std::isalpha(c)) return identifier();
	if (std::isdigit(c)) return number();

	switch (c)
	{
		case '(': return make_token(TokenType::LeftParen);
		case ')': return make_token(TokenType::RightParen);
		case '{': return make_token(TokenType::LeftBrace);
		case '}': return make_token(TokenType::RightBrace);
		case ';': return make_token(TokenType::Semicolon);
		case ',': return make_token(TokenType::Comma);
		case '.': return make_token(TokenType::Dot);
		case '-': return make_token(TokenType::Minus);
		case '+': return make_token(TokenType::Plus);
		case '/': return make_token(TokenType::Slash);
		case '*': return make_token(TokenType::Star);
		case '!':
			return make_token(match('=') ? TokenType::BangEqual : TokenType::Bang);
		case '=':
			return make_token(match('=') ? TokenType::EqualEqual : TokenType::Equal);
		case '<':
			return make_token(match('=') ? TokenType::LessEqual : TokenType::Less);
		case '>':
			return make_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
		case '"': return string();
	}
	return error_token("Unexpected character.");
}

Token Scanner::identifier()
{
	while (std::isalnum(peek()))
		advance();
	return make_token(identifier_type());
}

Token Scanner::number()
{
	while (std::isdigit(peek()))
		advance();

	if (peek() == '.' && std::isdigit(peek_next()))
	{
		advance();
		while (std::isdigit(peek()))
			advance();
	}

	return make_token(TokenType::Number);
}

Token Scanner::string()
{
	while (peek() != '"' && !is_at_end())
	{
		if (peek() == '\n') line++;
		advance();
	}

	if (is_at_end())
		return error_token("Unterminated string.");

	// closing quote
	advance();
	return make_token(TokenType::String);
}

void Scanner::skip_whitespace()
{
	while (true)
	{
		char c = peek();
		switch (c)
		{
			case ' ':
			case '\r':
			case '\t':
				advance();
				break;
			case '\n':
				line++;
				advance();
				break;
			case '/':
				if (peek_next() == '/')
				{
					while (peek() != '\n' && !is_at_end())
						advance();
				} else
				{
					return;
				}
				break;
			default:
				return;
		}
	}
}

TokenType Scanner::identifier_type() const
{
	switch (source.at(start))
	{
		case 'a': return check_keyword(1, 2, "nd", TokenType::And);
		case 'c': return check_keyword(1, 4, "lass", TokenType::Class);
		case 'e': return check_keyword(1, 3, "lse", TokenType::Else);
		case 'f':
			if (current - start > 1)
			{
				switch (source.at(start + 1))
				{
					case 'a': return check_keyword(2, 3, "lse", TokenType::False);
					case 'o': return check_keyword(2, 1, "r", TokenType::For);
					case 'u': return check_keyword(2, 1, "n", TokenType::Fun);
				}
			}
			break;
		case 'i': return check_keyword(1, 1, "f", TokenType::If);
		case 'n': return check_keyword(1, 2, "il", TokenType::Nil);
		case 'o': return check_keyword(1, 1, "r", TokenType::Or);
		case 'p': return check_keyword(1, 4, "rint", TokenType::Print);
		case 'r': return check_keyword(1, 5, "eturn", TokenType::Return);
		case 's': return check_keyword(1, 4, "uper", TokenType::Super);
		case 't':
			if (current - start > 1)
			{
				switch (source.at(start + 1))
				{
					case 'h': return check_keyword(2, 2, "is", TokenType::This);
					case 'r': return check_keyword(2, 2, "ue", TokenType::True);
				}
			}
			break;
		case 'v': return check_keyword(1, 2, "ar", TokenType::Var);
		case 'w': return check_keyword(1, 4, "hile", TokenType::While);
		default:
			break;
	}
	return TokenType::Identifier;
}

TokenType Scanner::check_keyword(size_t begin, size_t length, std::string_view rest, TokenType type) const
{
	if (current - start == begin + length
		&& source.substr(start + begin, length) == rest)
		return type;
	return TokenType::Identifier;
}

Token Scanner::error_token(std::string_view message) const noexcept
{
	return Token(TokenType::Error, message, line);
}

Token Scanner::make_token(TokenType type) const noexcept
{
	size_t length = current - start;
	return Token(type, source.substr(start, length), line);
}

char Scanner::advance()
{
	current++;
	return source.at(current - 1);
}

bool Scanner::is_at_end() const noexcept
{
	return current == source.length();
}

bool Scanner::match(char expected)
{
	if (is_at_end()) return false;
	if (source.at(current) != expected) return false;
	current++;
	return true;
}

char Scanner::peek() const
{
	if (is_at_end()) return '\0';
	return source.at(current);
}

char Scanner::peek_next() const
{
	if (is_at_end() || current + 1 == source.length())
		return '\0';
	return source.at(current + 1);
}

} //Clox