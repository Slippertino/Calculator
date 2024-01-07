#pragma once

#include <any>

namespace calculator {

enum class object_type {
    EXPR,
    OPERAND,
    OPERATOR
};

struct object_t {
    object_type type;
    std::any value;
};

} // namespace calculator
