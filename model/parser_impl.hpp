#include <vector>
#include <string>
#include <sstream>
#include <stack>
#include "lexer.hpp"
#include "op.hpp"
#include "parser.hpp"

namespace calculator {

namespace {

bool is_value(object_type ot) {
    return ot == object_type::OPERAND || ot == object_type::EXPR;
}

bool validate_operand(const std::vector<object_t>& objs, int id) {
    return !id || !is_value(objs[id - 1].type);
}

bool validate_unary(const std::vector<object_t>& objs, int id) {
    return !id || objs[id - 1].type == object_type::OPERATOR;
}

bool validate_binary(const std::vector<object_t>& objs, int id) {
    return id && objs[id - 1].type != object_type::OPERATOR;
}

bool validate_operator(const std::vector<object_t>& objs, int id) {
    auto op = std::any_cast<op_ptr>(objs[id].value);

    switch (op->category())
    {
    case op_category::UNARY:
        return validate_unary(objs, id);
    case op_category::BINARY:
        return validate_binary(objs, id);
    default:
        return false;
    }
}

bool validate_expression(const object_t& expr) {
    auto& childs = *std::any_cast<std::vector<object_t>>(&expr.value);

    for (auto i = 0; i < childs.size(); ++i) {
        bool res{ true };

        if (childs[i].type == object_type::OPERATOR)
            res = validate_operator(childs, i);
        else
            res = validate_operand(childs, i);

        if (!res)
            return false;
    }

    return true;
}

void add_op(std::vector<object_t>& expr, symbol_type symbol) {
    auto op = operations.at(symbol);

    switch (symbol)
    {
    case symbol_type::FACT:
    {
        if (expr.empty()) {
            expr.emplace_back(object_type::OPERATOR, op);
            break;
        }
        auto prev = expr.back();
        expr.pop_back();
        expr.emplace_back(object_type::OPERATOR, op);
        expr.push_back(std::move(prev));
        break;
    }
    case symbol_type::MINUS:
    {
        if (expr.empty()) {
            expr.emplace_back(object_type::OPERATOR, op);
            break;
        }
        auto type = expr.back().type;
        if (is_value(type))
            expr.emplace_back(object_type::OPERATOR, operations.at(symbol_type::ADD));
    }
    [[fallthrough]];
    default:
        expr.emplace_back(object_type::OPERATOR, op);
        break;
    }
}

} // namespace

template<is_lexer_like Lexer>
std::pair<object_t, status_type> parse(Lexer&& lex) {
    object_t out;
    out.type = object_type::EXPR;
    out.value = std::vector<object_t>{};

    std::stack<object_t*> nested_objs;
    nested_objs.push(&out);

    bool continue_{true};
    while (continue_) {
        if (nested_objs.empty())
            return { out, status_type::INVALID_EXPR };
            
        auto &obj = *nested_objs.top();
        auto &cur_objs = *std::any_cast<std::vector<object_t>>(&obj.value);

        auto cur = lex.get_token();
        if (cur.status != status_type::OK)
            return { out, cur.status };

        switch (cur.type)
        {
        case token_type::LBRACKET:
            cur_objs.push_back({ object_type::EXPR, std::vector<object_t>{}});
            nested_objs.push(&cur_objs.back());
            break;
        case token_type::RBRACKET:
            if (!validate_expression(obj))
                return { out, status_type::INVALID_EXPR };
            nested_objs.pop();
            break;
        case token_type::SYMBOL:
            {
                auto st = std::any_cast<symbol_type>(cur.value);
                if (constants.contains(st))
                    cur_objs.emplace_back(object_type::OPERAND, constants.at(st));
                else
                    add_op(cur_objs, st);
            }
            break;
        case token_type::NUMBER:
            cur_objs.emplace_back(
                object_type::OPERAND,
                constant(std::any_cast<number_t>(cur.value))
            );
            break;
        case token_type::EMPTY:
            [[fallthrough]];
        default:
            continue_ = false;
            break;
        }
    }

    return validate_expression(out) 
        ? std::pair{ out, status_type::PARTLY_INVALID_EXPR } 
        : std::pair{ out, status_type::INVALID_EXPR };
}

} // namespace calculator
