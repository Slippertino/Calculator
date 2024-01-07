#pragma once

#include <any>
#include "status.hpp"

namespace calculator {

enum class token_type {
    NUMBER,
    SYMBOL,
    LBRACKET,
    RBRACKET,
    EMPTY
};

struct token_t {
    token_type type;
    status_type status;
    std::any value;
};

const token_t empty_token = token_t{ token_type::EMPTY, status_type::OK, nullptr };

enum class symbol_type {
    UNKNOWN,
    ADD,
    MULT,
    DIV,
    POW,
    MINUS,
    FACT,
    MOD,
    COS,
    SIN,
    TAN,
    ACOS,
    ASIN,
    ATAN,
    LG,
    LN,
    SQRT,
    PI,
    E
};

} // namespace calculator