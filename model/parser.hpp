#pragma once

#include <sstream>
#include <concepts>
#include "object.hpp"
#include "status.hpp"
#include "token.hpp"

namespace calculator {

template<typename Lexer>
concept is_lexer_like = requires(Lexer lex) {
	{ lex.get_token() } -> std::same_as<token_t>;
};

template<is_lexer_like Lexer>
std::pair<object_t, status_type> parse(Lexer&& lex);

} // namespace calculator

#include "parser_impl.hpp"
