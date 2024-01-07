#pragma once

#include <list>
#include <array>
#include <functional>
#include <algorithm>
#include <QString>
#include "model/parser.hpp"
#include "model/eval.hpp"
#include "settings.hpp"
#include "elements.hpp"
#include "proxylexer.hpp"
#include "stdextension.hpp"

class Expression {
private:
    using ContainerWrapperType  = OptimizedListWrapper<BlockPtr, 128>;

public:
    using ContainerType         = typename ContainerWrapperType::ListType;
    using BlockPtrIt            = typename ContainerType::iterator;
    
private:
    struct Update {
        BlockPtrIt lastModified;
        int offset;
    };

public:
    Expression() = default;
    Expression(const Expression& ce) : 
        changed_{ true },
        current_position_{ ce.current_position_ }
    {
        for (auto& bl : ce.expr_)
            expr_.push_back(bl->clone());
    }

    Expression& operator=(const Expression& rhs) {
        if (this == &rhs)
            return *this;

        Expression tmp{ rhs };

        clear();
        expr_ = std::move(tmp.expr_);
        current_position_ = tmp.current_position_;
        changed_ = true;
        return *this;
    }

    QString getExpression() const {
        if (changed_) {
            changed_ = false;
            buildExpression();
        }

        return last_;
    }

    int getPosition() const noexcept {
        return current_position_;
    }

    int getOpenBracketsCount() const noexcept {
        auto cnt{ 0 };
        for (auto& bl : expr_) {
            auto type = bl->type();
            cnt += type == calculator::token_type::LBRACKET;
            cnt -= type == calculator::token_type::RBRACKET;
        }
        return cnt;
    }

    int size() const {
        auto lastit = expr_.rbegin();
        return (lastit != expr_.rend())
            ? (*lastit)->end()
            : 0;
    }

    auto begin() const {
        return expr_.begin();
    }

    auto end() const {
        return expr_.end();
    }

    void setPosition(int value) {
        if (value < 0)
            value = 0;

        auto sz = size();
        if (value > sz)
            value = sz;

        current_position_ = value;
    }

    Expression& pushFront(const QString& val) {
        return insertRange({ {0, val} });
    }

    Expression& pushBack(const QString& val) {
        return insertRange({ {size(), val} });
    }

    Expression& insert(const QString& val) {
        return insertRange({ { current_position_, val } });
    }

    Expression& insertRange(std::vector<std::pair<int, QString>> vals) {
        std::sort(vals.begin(), vals.end(), [](const auto& v1, const auto& v2) {
            return v1.first < v2.first; 
        });

        Update upd{ expr_.begin(), 0 };
        for (auto& v : vals) {
            auto offset = upd.offset;
            auto pos = v.first + offset;
            upd = insertOne(upd.lastModified, offset, pos, v.second);
            updatePosition(pos, upd.offset);
            upd.offset += offset;
        }
        shiftAll(std::next_or_default(upd.lastModified, expr_.end()), expr_.end(), upd.offset);
        update();

        return *this;
    }

    Expression& insertBlockRange(std::vector<std::pair<int, BlockPtr>> vals) {
        std::sort(vals.begin(), vals.end(), [](const auto& v1, const auto& v2) { 
            return v1.first < v2.first; 
        });

        Update upd{ expr_.begin(), 0 };
        for (auto& v : vals) {
            auto offset = upd.offset;
            auto pos = v.first + offset;
            auto next = selectFirstSuitable(upd.lastModified, offset, pos);
            auto bl_sz = v.second->size();
            auto start = (next != expr_.end()) ? (*next)->begin() : size();
            v.second->shift(-v.second->begin() + start);
            if (next != expr_.end())
                (*next)->shift(bl_sz);
            expr_.insert(next, std::move(v.second));
            updatePosition(start, bl_sz);
            upd.offset += bl_sz;
            upd.lastModified = next;
        }
        shiftAll(std::next_or_default(upd.lastModified, expr_.end()), expr_.end(), upd.offset);
        update();

        return *this;
    }

    Expression& popFront(int count) {
        return removeRange({ { 0, count } });
    }

    Expression& popBack(int count) {
        return removeRange({ { size() - count, count} });
    }

    Expression& remove(int count) {
        return removeRange({{ current_position_, count }});
    }

    Expression& removeRange(std::vector<std::pair<int, int>> vals) {
        std::sort(vals.begin(), vals.end(), [](const auto& v1, const auto& v2) {
            return v1.first < v2.first; 
        });

        Update upd{ expr_.begin(), 0 };
        for (auto& v : vals) {
            auto offset = upd.offset;
            auto pos = v.first - offset;
            upd = removeOne(upd.lastModified, offset, pos, v.second);
            updatePosition(pos, -upd.offset);
            upd.offset += offset;
        }
        shiftAll(std::next_or_default(upd.lastModified, expr_.end()), expr_.end(), -upd.offset);
        update();

        return *this;
    }

    Expression& removeBlockRange(std::vector<int> vals) {
        std::sort(vals.begin(), vals.end(), [](const auto& v1, const auto& v2) {
            return v1 < v2; 
        });

        Update upd{ expr_.begin(), 0 };
        for (auto& v : vals) {
            auto offset = upd.offset;
            auto pos = v - offset;
            auto cur = selectFirstSuitable(upd.lastModified, -offset, pos);
            if (cur == expr_.end())
                break;

            auto sz = (*cur)->size();
            updatePosition((*cur)->begin(), -sz);

            upd.lastModified = expr_.erase(cur);
            upd.offset += sz;
            if (upd.lastModified != expr_.end())
                (*upd.lastModified)->shift(-upd.offset);
        }
        shiftAll(std::next_or_default(upd.lastModified, expr_.end()), expr_.end(), -upd.offset);
        update();

        return *this;
    }

    std::tuple<Expression, calculator::status_type, calculator::op_ptr> eval() const {
        auto lexer = ProxyLexer{ *this };
        auto [obj, st] = calculator::parse(std::move(lexer));

        Expression expr;
        if (st != calculator::status_type::PARTLY_INVALID_EXPR)
            return { expr, st, nullptr };

        auto [res, est, op] = calculator::eval(obj, Settings::precision);
        if (est != calculator::status_type::OK)
            return { expr, est, op};

        expr.expr_.emplace_back(new Number(0, res));
        expr.current_position_ = expr.size();

        return { expr, est, nullptr};
    }

    std::pair<calculator::status_type, calculator::op_ptr> evalAndUpdate() {
        auto [res, st, op] = eval();

        if (st != calculator::status_type::OK)
            return { st, op };

        clear();
        expr_ = std::move(res.expr_);
        current_position_ = res.current_position_;
        update();

        return { st, nullptr };
    }

    int shiftPosition(int shift) {
        updatePosition(-1, shift);
        return current_position_;
    }

    int updatePosition(int pos, int shift) {
        if (current_position_ >= pos && shift >= 0)
            current_position_ += shift;
        else if (current_position_ > pos && shift < 0)
            current_position_ -= std::min(current_position_ - pos, -shift);
        return current_position_;
    }

    template<typename Formatter = void>
    void update() {
        last_.clear();
        changed_ = true;
        if constexpr (!std::is_same_v<Formatter, void>)
            applyFormatFeatures<Formatter>();
    }

    void clear() {
        expr_.clear();
        current_position_ = 0;
        update();
    }

private:
    BlockPtrIt getLast() {
        return std::next_or_default(expr_.rbegin(), expr_.rend(), expr_.rbegin()).base();
    }

    Update insertOne(BlockPtrIt last_modified, int offset, int pos, const QString& val) {
        BlockPtrIt 
            curit = selectFirstSuitable(last_modified, offset, pos),
            previt = std::prev_or_default(curit, expr_.begin(), expr_.end(), expr_.end(), getLast()),
            nextit = curit;

        Update upd;
        auto blocks = Block::create(pos, val);
        if (blocks.list.empty())
            return { curit, 0 };

        if (curit != expr_.end()) {
            upd = tryInsertToBlock(curit, pos, blocks);
            if (upd.offset)
                return upd;
        }

        if ((previt != expr_.end() && (*previt)->end() != pos) ||
            (curit != expr_.end() && (*curit)->begin() != pos))
            return { curit, 0 };

        OptimizedListWrapper<BlockPtrIt, 2> delete_lst;
        if (previt != expr_.end() && (*previt)->isMutable()) {
            blocks.list.push_front((*previt)->clone());
            delete_lst.list.push_back(previt);
        }

        if (curit != expr_.end() && (*curit)->isMutable()) {
            blocks.list.push_back((*curit)->clone());
            nextit = std::next_or_default(curit, expr_.end(), expr_.end());
            delete_lst.list.push_back(curit);
        }

        upd = mergeInto(nextit, blocks);
        for (auto& it : delete_lst)
            expr_.erase(it);

        return upd;
    }

    Update removeOne(BlockPtrIt last_modified, int offset, int pos, int count) {
        auto curit = selectFirstSuitable(last_modified, -offset, pos);
        if (curit == expr_.end())
            return { curit, 0 };

        auto total{ 0 };
        while (total < count) {
            auto& cur = *curit;
            auto cnt = cur->remove(pos, count - total);
            total += cnt;

            if (cur->isInside(pos))
                return { curit, total };

            bool empty = cur->empty();
            if (empty) {
                pos = cur->begin();
                curit = expr_.erase(curit);
            }
            else if (total < count)
                curit = std::next(curit);

            if (curit == expr_.end())
                return { curit, total };

            if (empty || total < count) 
                (*curit)->shift(-(total + offset));
        }

        if (curit == expr_.begin())
            return { curit, total };

        BlockPtrIt
            previt{ std::prev(curit) },
            nextit{ std::next(curit) };

        if (!(*previt)->isMutable())
            return { curit, total };

        BlockPtrList lst;
        lst.list.push_back((*previt)->clone());
        lst.list.push_back((*curit)->clone());
        auto res = mergeInto(nextit, lst);
        expr_.erase(previt);
        expr_.erase(curit);

        res.offset = total - res.offset;
        return res;
    }

    BlockPtrIt selectFirstSuitable(BlockPtrIt last_modified, int offset, int pos) {
        auto end = expr_.end();

        if (last_modified == end || (*last_modified)->isInside(pos))
            return last_modified;

        for (auto it = std::next(last_modified); it != end; ++it) {
            auto& cur = *it;
            cur->shift(offset);
            if (cur->isInside(pos))
                return it;
        }

        return end;
    }

    Update tryInsertToBlock(BlockPtrIt block_it, int pos, BlockPtrList& bucket) {
        auto& bucketList = bucket.list;

        Update res{ block_it, 0 };
        auto& shift = res.offset;

        auto& block = *block_it;
        while (!bucketList.empty()) {
            auto cur = bucketList.front()->clone();
            auto sh = block->insert(pos + shift, *cur);
            if (!sh) break;
            shift += sh;
            bucketList.pop_front();
        }
        pos += shift;
        if (bucketList.empty() || !block->canSplit(pos))
            return res;

        auto [vbl, vbr, soff] = block->split(pos);
        bucketList.insert(
                bucket.begin(),
                std::make_move_iterator(vbl.begin()),
                std::make_move_iterator(vbl.end())
        );
        bucketList.insert(
                bucket.end(),
                std::make_move_iterator(vbr.begin()),
                std::make_move_iterator(vbr.end())
        );

        auto next = std::next(block_it);
        res = mergeInto(next, bucket);
        expr_.erase(block_it);

        return res;
    }

    Update mergeInto(BlockPtrIt dest, BlockPtrList& list) {
        Update res;
        auto begin = list.list.front()->begin();
        auto end = (dest == expr_.end()) ? size() : (*dest)->begin();
        auto blocks = Block::merge(begin, list);
        res.offset = blocks.list.back()->end() - end;
        for (auto& b : blocks)
            res.lastModified = expr_.insert(dest, std::move(b));
        return res;
    }

    void shiftAll(BlockPtrIt begin, BlockPtrIt end, int val) {
        std::for_each(begin, end, [&val](auto &ptr) { ptr->shift(val);});
    }

    template<typename Formatter>
    void applyFormatFeatures() {
        for (auto f : Formatter(*this, expr_))
            for (auto it = expr_.begin(); it != expr_.end(); ++it)
                f(it);
    }

    void buildExpression() const {
        last_.clear();
        for(auto &bl : expr_)
            last_.append(bl->toString());
    }

private:
    mutable QString last_;
    mutable bool changed_{ true };

    int current_position_{ 0 };
    ContainerWrapperType exprWrapper_;
    ContainerType& expr_ = exprWrapper_.list;
};
