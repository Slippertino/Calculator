#pragma once

#include <list>
#include "model/token.hpp"
#include "model/lexer.hpp"
#include "settings.hpp"
#include "elements.hpp"

template<typename Iter>
class ProxyLexer {
public:
	ProxyLexer() = delete;

    template<class ExprT>
    ProxyLexer(ExprT &expr) : current_{expr.begin()}, end_{expr.end()}
	{ }

	calculator::token_t get_token() {
        while (current_ != end_ && is_empty())
			current_ = std::next(current_);

		if (current_ == end_)
			return calculator::empty_token;

		auto& cur = *current_;
		current_ = std::next(current_);

		if (cur->type() == calculator::token_type::NUMBER) {
			auto ptr = static_cast<Number*>(cur.get());
            if (!ptr->isMutable())
				return calculator::token_t {
					calculator::token_type::NUMBER,
					calculator::status_type::OK,
					ptr->get()
				};
		}

		calculator::lexer lex(cur->toString(false).toStdWString(), Settings::max_output_size);
        return lex.get_token();
	}

private:
	bool is_empty() const noexcept {
		return (*current_)->type() == calculator::token_type::EMPTY;
	}

private:
    Iter current_, end_;
};

template<class ExprT>
ProxyLexer(ExprT &expr) -> ProxyLexer<decltype(std::declval<ExprT>().begin())>;