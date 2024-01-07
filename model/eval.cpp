#include <vector>
#include <stack>
#include "eval.hpp"

namespace calculator {

struct eval_context {
    std::stack<number_t> nums;
    std::stack<op_ptr> ops;
    object_t expr;
    int i;

    eval_context() = default;
    eval_context(const object_t &obj) : expr{obj}, i{-1}
    { }
};

std::vector<number_t> extract_args(std::stack<number_t> &src, op_category op) {
    std::vector<number_t> res;

    switch (op)
    {
    case op_category::BINARY: 
        {
            if (src.size() < 2)
                break;
            auto sec = src.top(); src.pop();
            res.push_back(src.top());
            src.pop();
            res.push_back(sec);
        }
        break;
    case op_category::UNARY:
        if (src.empty())
            break;
        res.push_back(src.top());
        src.pop();
        break;
    default:
        break;
    }

    return res;
}

std::tuple<bool, status_type, op_ptr> eval_expression(eval_context &ec, number_t &result, int prec) {
    auto &nums = ec.nums;
    auto &ops = ec.ops;
    auto &expr = ec.expr;
    auto &childs = *std::any_cast<std::vector<object_t>>(&expr.value);

    op_ptr curop;
    for(auto i = ec.i + 1; i < childs.size(); ++i) {
        auto &o = childs[i];
        switch (o.type)
        {
        case object_type::EXPR:
            ec.i = i;
            return { false, status_type::OK, nullptr };
        case object_type::OPERAND: 
            {
                auto [num, st] = std::any_cast<constant>(o.value).exec({});
                num.set_prec(prec);
                nums.push(std::move(num));     
            }
            break;
        case object_type::OPERATOR: 
            {
                auto op = std::any_cast<op_ptr>(o.value);
                while(!ops.empty() && ops.top()->priority() >= op->priority()) {
                    curop = ops.top(); ops.pop();
                    auto [res, status] = curop->exec(extract_args(nums, curop->category()));
                    if (status != status_type::OK)
                        return { false, status, curop };
                    nums.push(res);
                }
                ops.push(op); 
            }
            break;
        default:
            break;
        }
    }

    while(!ops.empty()) {
        auto curop = ops.top(); ops.pop();
        auto [res, status] = curop->exec(extract_args(nums, curop->category()));
        if (status != status_type::OK)
            return { false, status, curop };
        nums.push(res);
    }

    if (nums.empty())
        return { false, status_type::INVALID_EVAL, nullptr };

    result = nums.top();

    return { true, status_type::OK, curop };
}

std::tuple<number_t, status_type, op_ptr> eval(const object_t &expression, int prec) {
    std::stack<eval_context> conts;
    bool eval_ready{false};
    number_t result{0};

    conts.emplace(expression);
    while(!conts.empty()) {
        auto &cont = conts.top();

        if (eval_ready) {
            cont.nums.push(result);
            eval_ready = false;
        }

        auto [fin, status, op] = eval_expression(cont, result, prec);
        if (status != status_type::OK)
            return { result, status, op };

        if (fin) {
            eval_ready = true;
            conts.pop();
        }
        else {
            auto &childs = *std::any_cast<std::vector<object_t>>(&cont.expr.value);
            conts.emplace(childs[cont.i]);
        }
    }

    return { 
        result, 
        status_type::OK, 
        nullptr
    };
}

} // namespace calculator
