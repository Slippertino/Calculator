#pragma once

namespace calculator {

enum class status_type {
    OK,
    UNKNOWN_ERROR,

    // lexer's errors
    TOO_LONG_NUMBER,
    INVALID_NUMBER,
    UNKNOWN_SYMBOL,

    // parser's errors
    PARTLY_INVALID_EXPR,
    INVALID_EXPR,

    // eval's errors
    INVALID_EVAL,
    NUMBER_OVERFLOW,
    INVALID_ARGUMENT
};

} // namespace calculator