#pragma once

#include <memory>
#include <functional>
#include <numbers>
#include "status.hpp"
#include "lexer.hpp"

namespace calculator {

struct computable {
    using result_type = std::pair<number_t, status_type>;
    virtual result_type exec(const std::vector<number_t>& args) const = 0;
    virtual ~computable() = default;
};

class constant final : public computable {
public:
    constant() = default;

    template<typename T>
    constant(const T &arg) : value_{ arg }
    { }

    result_type exec(const std::vector<number_t>& args) const override final {
        return { value_, status_type::OK };
    }

private:
    const number_t value_;
};

enum class op_category {
    UNARY = 1,
    BINARY
};

class operation : public computable {
public:
    operation() = delete;
    operation(symbol_type type, op_category category, int priority) : 
        type_(type),
        category_(category),
        priority_(priority)
    { }

    symbol_type type() const noexcept {
        return type_;
    }

    op_category category() const noexcept {
        return category_;
    }

    int priority() const noexcept {
        return priority_;
    }

    result_type exec(const std::vector<number_t> &args) const override final {
        if (args.size() < static_cast<size_t>(category_))
            return { 0, status_type::INVALID_EVAL };

        auto [res, status] = exec_impl(args);

        if (status != status_type::OK)
            return { res, status };
        
        if (!isfinite(res))
            return { 0, status_type::INVALID_ARGUMENT };

        if (res > k_max_value_)
            return { 0, status_type::NUMBER_OVERFLOW };
     
        return { res, status };
    }

    virtual ~operation() = default;

protected:
    virtual result_type  exec_impl(const std::vector<number_t> &args) const = 0;

protected: 
    inline static const number_t k_max_value_{"1e+100"};

private:
    symbol_type type_;
    op_category category_;
    int priority_;
};

#define DECLARE_TRIVIAL_OPERATION(name, type, category, func) \
class name final : public operation { \
public: \
    name() = delete; \
    name(int priority) : operation(type, category, priority) { } \
private: \
    result_type exec_impl(const std::vector<number_t>& args) const override final { \
        func \
    } \
}; 
#define RESULT(arg1, arg2) result_type (arg1, arg2)

DECLARE_TRIVIAL_OPERATION(addition, symbol_type::ADD, op_category::BINARY,
    return RESULT(args[0] + args[1], status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(minus, symbol_type::MINUS, op_category::UNARY,
    number_t nm = args[0];
    return RESULT(mpfr::negate(nm), status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(multiplication, symbol_type::MULT, op_category::BINARY,
    return RESULT(args[0] * args[1], status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(division, symbol_type::DIV, op_category::BINARY,
    return RESULT(args[0] / args[1], status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(pow, symbol_type::POW, op_category::BINARY,
    return RESULT(mpfr::pow(args[0], args[1]), status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(sqrt, symbol_type::SQRT, op_category::UNARY,
    return RESULT(mpfr::sqrt(args[0]), status_type::OK);
)

class factorial final : public operation {
public:
    factorial() = delete;
    factorial(int priority) : operation(symbol_type::FACT, op_category::UNARY, priority)
    { }

private:
    result_type exec_impl(const std::vector<number_t>& args) const override final {
        const auto &arg{ args[0] };
        if (!mpfr::isint(arg))
            return { 0, status_type::INVALID_ARGUMENT };

        number_t res{ 1 }, mult{ 1 };
        for (number_t i = 1; i <= arg; ++i) {
            res *= i;
            if (res > k_max_value_)
                return { 0, status_type::NUMBER_OVERFLOW };
        }

        return { res, status_type::OK };
    }
};

DECLARE_TRIVIAL_OPERATION(mod, symbol_type::MOD, op_category::BINARY,
    if (!mpfr::isint(args[0]) || !mpfr::isint(args[1]))
        return RESULT(0, status_type::INVALID_ARGUMENT);
    return RESULT(mpfr::mod(args[0], args[1]), status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(cos, symbol_type::COS, op_category::UNARY,
    return RESULT(mpfr::cos(args[0]), status_type::OK);
)
DECLARE_TRIVIAL_OPERATION(sin, symbol_type::SIN, op_category::UNARY,
    return RESULT(mpfr::sin(args[0]), status_type::OK);
)
DECLARE_TRIVIAL_OPERATION(tan, symbol_type::TAN, op_category::UNARY,
    return RESULT(mpfr::tan(args[0]), status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(acos, symbol_type::ACOS, op_category::UNARY,
    return RESULT(mpfr::acos(args[0]), status_type::OK);
)
DECLARE_TRIVIAL_OPERATION(asin, symbol_type::ASIN, op_category::UNARY,
    return RESULT(mpfr::asin(args[0]), status_type::OK);
)
DECLARE_TRIVIAL_OPERATION(atan, symbol_type::ATAN, op_category::UNARY,
    return RESULT(mpfr::atan(args[0]), status_type::OK);
)

DECLARE_TRIVIAL_OPERATION(ln, symbol_type::LN, op_category::UNARY,
    return RESULT(mpfr::log(args[0]), status_type::OK);
)
DECLARE_TRIVIAL_OPERATION(lg, symbol_type::LG, op_category::UNARY,
    return RESULT(mpfr::log10(args[0]), status_type::OK);
)

using op_ptr         = std::shared_ptr<operation>;
inline const std::unordered_map<symbol_type, op_ptr> operations = {
    { symbol_type::FACT,            std::make_shared<factorial>(4)          },
    { symbol_type::POW,             std::make_shared<pow>(3)                },
    { symbol_type::MINUS,           std::make_shared<minus>(2)              },
    { symbol_type::SQRT,            std::make_shared<sqrt>(2)               },
    { symbol_type::MOD,             std::make_shared<mod>(2)                },
    { symbol_type::COS,             std::make_shared<cos>(2)                },
    { symbol_type::SIN,             std::make_shared<sin>(2)                },
    { symbol_type::TAN,             std::make_shared<tan>(2)                },
    { symbol_type::ACOS,            std::make_shared<acos>(2)               },
    { symbol_type::ASIN,            std::make_shared<asin>(2)               },
    { symbol_type::ATAN,            std::make_shared<atan>(2)               },
    { symbol_type::LN,              std::make_shared<ln>(2)                 },
    { symbol_type::LG,              std::make_shared<lg>(2)                 },
    { symbol_type::MULT,            std::make_shared<multiplication>(1)     },
    { symbol_type::DIV,             std::make_shared<division>(1)           },
    { symbol_type::ADD,             std::make_shared<addition>(0)           },
};

inline const std::unordered_map<symbol_type, constant> constants = {
    { symbol_type::PI,              constant(mpfr::const_pi())      },
    { symbol_type::E,               constant(mpfr::exp(1))          },
};

} // namespace calculator
