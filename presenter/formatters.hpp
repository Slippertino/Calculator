#pragma once

#include <array>
#include <vector>
#include <functional>
#include <QRegularExpression>
#include "model/op.hpp"
#include "elements.hpp"

template<class ExprT, class ContT>
struct FormatterBase {
    using iterator_t  = typename ContT::iterator;

    ExprT &expr_;
    ContT &cont_;
    std::vector<std::pair<int, BlockPtr>> to_insert_;
    std::vector<int> to_remove_;

    FormatterBase() = delete;
    FormatterBase(ExprT &expr, ContT &cont) :
        expr_{ expr }, 
        cont_{ cont }
    { }

    FormatterBase(const FormatterBase& fb) :
        FormatterBase(fb.expr_, fb.cont_)
    { }

    ~FormatterBase() {
        expr_.insertBlockRange(std::move(to_insert_));
        expr_.removeBlockRange(std::move(to_remove_));
    }
};

template<int Id>
struct FormatterOrder {
    static_assert(Id < Block::kMaxFormattersCount && "too large formatter's id");
    static constexpr int id = Id;
};

template<class ExprT, template<typename... Args> typename... Formatters>
struct Formatter {
    using container_t = typename ExprT::ContainerType;
    using func_t = std::function<void(typename FormatterBase<ExprT, container_t>::iterator_t)>;

    ExprT& expr_;
    container_t& cont_;
    const std::array<func_t, sizeof... (Formatters)> formatters_;

    Formatter() = delete;
    Formatter(ExprT& expr, container_t& cont) : 
        expr_{ expr },
        cont_{ cont },
        formatters_ { func_t{Formatters<ExprT, container_t>{expr, cont}}...  }
    { }

    auto begin() { 
        return formatters_.begin(); 
    }

    auto end() { 
        return formatters_.end(); 
    }
};

template<class ExprT, class ContT>
struct RightBracketComplementer : FormatterBase<ExprT, ContT>, FormatterOrder<0> {
    using typename FormatterBase<ExprT, ContT>::iterator_t;
    using FormatterBase<ExprT, ContT>::expr_;
    using FormatterBase<ExprT, ContT>::cont_;
    using FormatterBase<ExprT, ContT>::to_insert_;
    using FormatterBase<ExprT, ContT>::to_remove_;

    int opened_{ 0 };

    RightBracketComplementer() = delete;
    RightBracketComplementer(ExprT& expr, ContT& cont) : FormatterBase<ExprT, ContT>(expr, cont) { }

    void operator()(iterator_t it) {
        auto& cur = *it;
        opened_ += cur->type() == calculator::token_type::LBRACKET;
        opened_ -= cur->type() == calculator::token_type::RBRACKET;
        if (std::next(it) == cont_.end()) {
            for (auto i = 0; i < opened_; ++i)
                to_insert_.push_back({ expr_.size(), BlockPtr{ new RightBracket() } });
        }
    }
};

template<class ExprT, class ContT>
struct OperationComplementer : FormatterBase<ExprT, ContT>, FormatterOrder<2> {
    using typename FormatterBase<ExprT, ContT>::iterator_t;
    using FormatterBase<ExprT, ContT>::expr_;
    using FormatterBase<ExprT, ContT>::cont_;
    using FormatterBase<ExprT, ContT>::to_insert_;
    using FormatterBase<ExprT, ContT>::to_remove_;

    int offset_{ 0 };

    OperationComplementer() = delete;
    OperationComplementer(ExprT& expr, ContT& cont) : FormatterBase<ExprT, ContT>(expr, cont) { }

    void operator()(iterator_t it) {
        auto& cur = *it;
        auto& flags = cur->formatFlags();
        if (cur->type() != calculator::token_type::SYMBOL)
            return;

        auto symbol = *static_cast<Symbol*>(cur.get());
        auto prev_type = symbol.symbol_type();
        auto pos = symbol.begin() + offset_;
        auto diff = symbol.refresh();
        auto cur_type = symbol.symbol_type();
        auto upd_sz = symbol.size();

        if (flags.test(id)) {
            if (cur_type != calculator::symbol_type::UNKNOWN) {
                if (prev_type != calculator::symbol_type::UNKNOWN || 
                    prev_type == calculator::symbol_type::UNKNOWN && diff)
                    flags.reset(id);
            }
        } else {
            if (cur_type != calculator::symbol_type::UNKNOWN)
                return;

            if (prev_type == calculator::symbol_type::UNKNOWN)
                flags.set(id);
        }

        if (!diff)
            return;

        auto block = BlockPtr{ new Symbol(symbol) };
        block->formatFlags() = flags;

        if (diff > 0) {        
            to_insert_.push_back({ pos, std::move(block)});
            to_remove_.push_back(pos + upd_sz);
            offset_ += upd_sz;
        }
        else {
            if (upd_sz) {
                to_insert_.push_back({ pos, std::move(block)});
                pos += upd_sz;
                offset_ += upd_sz;
            }
            to_remove_.push_back(pos);
        }
    }
};

template<class ExprT, class ContT>
struct BinaryOperationSpaceComplementer : FormatterBase<ExprT, ContT>, FormatterOrder<3> {
    using typename FormatterBase<ExprT, ContT>::iterator_t;
    using FormatterBase<ExprT, ContT>::expr_;
    using FormatterBase<ExprT, ContT>::cont_;
    using FormatterBase<ExprT, ContT>::to_insert_;

    BinaryOperationSpaceComplementer() = delete;
    BinaryOperationSpaceComplementer(ExprT& expr, ContT& cont) : FormatterBase<ExprT, ContT>(expr, cont) { }

    void operator()(iterator_t it) {
        auto& cur = *it;
        auto& flags = cur->formatFlags();
        if (cur->type() != calculator::token_type::SYMBOL ||
            !flags.test(id))
            return;

        auto symbol = static_cast<Symbol*>(cur.get());
        auto stype = symbol->symbol_type();
        if (calculator::operations.find(stype) == 
            calculator::operations.end())
            return;

        auto op = calculator::operations.at(stype);
        if (op->category() != calculator::op_category::BINARY && 
            op->type() != calculator::symbol_type::MINUS)
            return;

        if (op->type() == calculator::symbol_type::MINUS) {
            auto previt = std::prev_or_default(it, cont_.begin(), cont_.end(), cont_.end());
            if (previt == cont_.end() || (*previt)->type() != calculator::token_type::NUMBER)
                return;
        }

        if (it != cont_.begin() && (*std::prev(it))->type() != calculator::token_type::EMPTY)
            to_insert_.push_back({ cur->begin(), BlockPtr{ new Space() }});

        auto next_it = std::next(it);
        if (next_it == cont_.end() || (*next_it)->type() != calculator::token_type::EMPTY)
            to_insert_.push_back({ cur->end(), BlockPtr{ new Space() }});

        flags.reset(id);
    }
};

template<class ExprT, class ContT>
struct UnaryOperationLeftBracketComplementer : FormatterBase<ExprT, ContT>, FormatterOrder<4> {
    using typename FormatterBase<ExprT, ContT>::iterator_t;
    using FormatterBase<ExprT, ContT>::expr_;
    using FormatterBase<ExprT, ContT>::cont_;
    using FormatterBase<ExprT, ContT>::to_insert_;
    using FormatterBase<ExprT, ContT>::to_remove_;

    UnaryOperationLeftBracketComplementer() = delete;
    UnaryOperationLeftBracketComplementer(ExprT& expr, ContT& cont) : FormatterBase<ExprT, ContT>(expr, cont) { }

    void operator()(iterator_t it) {
        auto& cur = *it;
        auto& flags = cur->formatFlags();
        if (cur->type() != calculator::token_type::SYMBOL ||
            !flags.test(id))
            return;

        auto symbol = static_cast<Symbol*>(cur.get());
        auto stype = symbol->symbol_type();
        if (calculator::operations.find(stype) == 
            calculator::operations.end())
            return;

        auto op = calculator::operations.at(stype);
        if (op->category() != calculator::op_category::UNARY ||
            op->type() == calculator::symbol_type::MINUS || 
            op->type() == calculator::symbol_type::FACT)
            return;

        auto next_it = std::next(it);
        if (next_it == cont_.end() || (*next_it)->type() != calculator::token_type::LBRACKET)
            to_insert_.push_back({ cur->end(), BlockPtr{ new LeftBracket() } });
        flags.reset(id);
    }
};

template<class ExprT, class ContT>
struct MinusComplementer : FormatterBase<ExprT, ContT>, FormatterOrder<5> {
    using typename FormatterBase<ExprT, ContT>::iterator_t;
    using FormatterBase<ExprT, ContT>::expr_;
    using FormatterBase<ExprT, ContT>::cont_;
    using FormatterBase<ExprT, ContT>::to_insert_;
    using FormatterBase<ExprT, ContT>::to_remove_;

    MinusComplementer() = delete;
    MinusComplementer(ExprT& expr, ContT& cont) : FormatterBase<ExprT, ContT>(expr, cont) { }

    void operator()(iterator_t it) {
        auto& cur = *it;
        if (cur->type() != calculator::token_type::SYMBOL)
            return;

        auto symbol = static_cast<Symbol*>(cur.get());
        auto stype = symbol->symbol_type();
        if (calculator::operations.find(stype) ==
            calculator::operations.end())
            return;

        auto op = calculator::operations.at(stype);
        if (op->type() != calculator::symbol_type::MINUS || it == cont_.begin())
            return;

        auto prev_it = std::prev(it);
        while ((*prev_it)->type() == calculator::token_type::EMPTY)
            prev_it = std::prev_or_default(prev_it, cont_.begin(), cont_.end(), cont_.end());

        if (prev_it == expr_.end())
            return;
        
        auto& prev = *prev_it;
        if (prev->type() != calculator::token_type::SYMBOL)
            return;

        auto prev_symbol = static_cast<Symbol*>(prev.get());
        auto prev_stype = prev_symbol->symbol_type();

        if (calculator::operations.find(prev_stype) == 
            calculator::operations.end())
            return;

        auto prev_op = calculator::operations.at(prev_stype);
        if (prev_op->category() != calculator::op_category::BINARY)
            return;

        to_insert_.push_back({ cur->begin(), BlockPtr{ new LeftBracket() }});
    }
};

template<class ExprT, class ContT>
struct NumberGapFormatter : FormatterBase<ExprT, ContT>, FormatterOrder<6> {
    using typename FormatterBase<ExprT, ContT>::iterator_t;
    using FormatterBase<ExprT, ContT>::expr_;
    using FormatterBase<ExprT, ContT>::cont_;
    using FormatterBase<ExprT, ContT>::to_insert_;
    using FormatterBase<ExprT, ContT>::to_remove_;

    static inline const QString             kGapValue = QStringLiteral(" ");
    static constexpr int                    kGapSize = 3;
    static inline const QRegularExpression  kFractPartMask = QRegularExpression(QStringLiteral("[.eE]"));

    int offset_{ 0 };

    NumberGapFormatter() = delete;
    NumberGapFormatter(ExprT& expr, ContT& cont) : FormatterBase<ExprT, ContT>(expr, cont) { }

    void operator()(iterator_t it) { 
        auto& cur = *it;
        auto cursor = expr_.getPosition();
        auto inside = cur->isInside(cursor) || (std::next(it) == expr_.end() && cursor == expr_.size());
        if (inside)
            cursor = expr_.shiftPosition(offset_);
        cur->shift(offset_);
        if (cur->type() != calculator::token_type::NUMBER)
            return;

        if (inside)
            expr_.shiftPosition(-cur->getDelimetersSizeByPos(cursor));
        offset_ += cur->clearFromDelimeters();
        QString str = cur->toString();
        auto order_pos = str.indexOf(kFractPartMask);
        if (order_pos != -1)
            str.remove(order_pos, str.size() - order_pos);

        auto mod = str.size() % kGapSize;
        auto gsz = kGapValue.size();
        auto cur_sz = str.size();
        auto insert_pos = (!mod ? kGapSize : mod) - gsz;
        while (insert_pos < cur_sz - gsz) {
            insert_pos += gsz;
            cur->insertDelimeter(insert_pos, kGapValue);
            if (inside)
                expr_.updatePosition(insert_pos, gsz);
            insert_pos += kGapSize;
            cur_sz += gsz;
            offset_ += gsz;
        }
    }
};

template<class ExprT, class ContT>
struct Reformatter : FormatterBase<ExprT, ContT>, FormatterOrder<7> {
    using typename FormatterBase<ExprT, ContT>::iterator_t;
    using FormatterBase<ExprT, ContT>::expr_;
    using FormatterBase<ExprT, ContT>::cont_;
    using FormatterBase<ExprT, ContT>::to_remove_;

    int offset_{ 0 };

    Reformatter() = delete;
    Reformatter(ExprT& expr, ContT& cont) : FormatterBase<ExprT, ContT>(expr, cont) { }

    void operator()(iterator_t it) {
        auto& cur = *it;
        cur->shift(-offset_);
        cur->formatFlags() = Block::kFullFlags;
        if (cur->type() == calculator::token_type::EMPTY) {
            to_remove_.push_back(cur->begin());
            return;
        }
        auto cursor = expr_.getPosition();
        expr_.shiftPosition(-cur->getDelimetersSizeByPos(cursor));
        offset_ += cur->clearFromDelimeters();
    }
};