#include <tuple>
#include "elements.hpp"

int DelimetersContainer::getDelimetersSizeByPos(int pos) const {
    int res{ 0 };
    for (auto& dl : delimeters_) {
        if (dl.pos >= pos)
            break;
        res += std::min(pos - dl.pos, dl.size);
    }
    return res;
}

DelimetersContainer::DelimetersContainer(QString& value, const std::list<Delimeter>& delims) :
    value_{ value },
    delimeters_{ delims }
{ }

void DelimetersContainer::insertDelimeter(int pos, const QString& val) {
    auto curit = find(pos);
    if (curit == delimeters_.end())
        pos = std::min(value_.size(), pos);
    else if (curit->isInside(pos))
        pos = curit->pos;
    value_.insert(pos, val);
    delimeters_.insert(curit, { pos, val.size() });
    shiftDelimeters(curit, val.size());
}

std::list<DelimetersContainer::Delimeter>::iterator DelimetersContainer::removeDelimeter(std::list<Delimeter>::iterator it, int offset) {
    offset += it->size;
    value_.remove(it->pos, it->size);
    auto next = delimeters_.erase(it);
    shiftDelimeters(next, -offset);
    return next;
}

void DelimetersContainer::clearFromDelimeters() {
    value_ = removeDelimeters();
    delimeters_.clear();
}

std::list<DelimetersContainer::Delimeter>::iterator DelimetersContainer::find(int pos) {
    return std::find_if(
        delimeters_.begin(), delimeters_.end(),
        [pos](auto& delim) {
            return pos < (delim.pos + delim.size);
        }
    );
}

void DelimetersContainer::shiftDelimeters(std::list<Delimeter>::iterator begin, int offset) {
    std::for_each(begin, delimeters_.end(), [offset](auto& delim) { delim.pos += offset;  });
}

QString DelimetersContainer::removeDelimeters() const {
    auto cur = value_;
    for (auto it = delimeters_.rbegin(); it != delimeters_.rend(); ++it) {
        auto& v = *it;
        cur.remove(v.pos, v.size);
    }
    return cur;
}

Block::Block(
    int start,
    calculator::token_type type,
    const QString& value,
    bool isMutable,
    format_t formatFlags,
    const std::list<Delimeter> &delimeters
) :
    mutable_{ isMutable },
    type_{ type },
    start_{ start },
    size_{ value.size() },
    format_flags_{ formatFlags },
    value_{ value },
    DelimetersContainer(value_, delimeters)
{ }

Block::Block(const Block& bl) :
    Block(bl.start_, bl.type_, bl.value_, mutable_, format_flags_)
{ }

BlockPtrList Block::create(int start, const QString& str) {
    BlockPtrList resWrapper;
    auto& res = resWrapper.list;

    // insert dummy
    res.push_back(std::make_unique<Space>(start - Space(0).size()));

    calculator::lexer lex(str.toStdWString(), Settings::max_output_size);

    auto prev{0};
    auto token = lex.get_token();
    while(prev != lex.get_current_position()) {
        auto value = QString::fromStdWString(lex.get_last());
        auto cur_pos = lex.get_current_position();
        for(auto i = prev; i < cur_pos - value.size(); ++i)
            res.push_back(std::make_unique<Space>(start + i));

        auto nstart = res.back()->end();
        switch(token.type)
        {
        case calculator::token_type::SYMBOL:
            res.push_back(std::make_unique<Symbol>(
                nstart,
                std::move(value),
                std::any_cast<calculator::symbol_type>(token.value))
            );
            break;
        case calculator::token_type::NUMBER:
            res.push_back(std::make_unique<Number>(nstart, std::move(value)));
            break;
        case calculator::token_type::LBRACKET:
            res.push_back(std::make_unique<LeftBracket>(nstart));
            break;
        case calculator::token_type::RBRACKET:
            res.push_back(std::make_unique<RightBracket>(nstart));
            break;
        default:
            break;
        }

        if (res.back()->empty())
            res.pop_back();

        prev = cur_pos;
        token = lex.get_token();
    }

    res.pop_front();
    return resWrapper;
}

BlockPtrList Block::merge(int start, BlockPtrList& blocks) {
    BlockPtrList res;
    auto& rlist = res.list;

    int cur{ start };
    rlist.push_back(std::make_unique<Space>(start - Space(0).size()));
    for (auto& bl : blocks) {
        bl->shift(-bl->begin() + cur);
        if (!rlist.back()->merge(*bl))
            rlist.push_back(std::move(bl));
        cur = rlist.back()->end();
    }
    rlist.pop_front();
    return res;
}

int Block::begin() const noexcept {
    return start_;
}

int Block::end() const noexcept {
    return start_ + size_;
}

int Block::size() const noexcept {
    return size_;
}

bool Block::empty() const noexcept {
    return !size_;
}

calculator::token_type Block::type() const noexcept {
    return type_;
}

Block::format_t& Block::formatFlags() noexcept {
    return format_flags_;
}

void Block::shift(int size) noexcept {
    start_ += size;
}

bool Block::isInside(int pos) const noexcept {
    return begin() <= pos && pos < end();
}

bool Block::canSplit(int pos) const noexcept {
    if (!isMutable())
        return false;

    return begin() < pos && pos < end();
}

std::tuple<BlockPtrList, BlockPtrList, int> Block::split(int pos) {
    if (!canSplit(pos))
        return { BlockPtrList{}, BlockPtrList{}, 0};

    pos -= start_;
    auto sz = size_;
    auto p1 = value_.left(pos);
    auto p2 = value_.right(value_.size() - pos);

    auto vb1 = Block::create(start_, p1);
    auto vb2 = Block::create(vb1.list.back()->end(), p2);

    auto diff = vb2.list.back()->end() - sz;

    return {
        std::move(vb1),
        std::move(vb2),
        diff
    };
}

int Block::insertDelimeter(int pos, const QString &delim) {
    DelimetersContainer::insertDelimeter(pos, delim);
    update(value_);
    return delim.size();
}

int Block::clearFromDelimeters() {
    auto old = value_;
    DelimetersContainer::clearFromDelimeters();
    update(value_);
    return value_.size() - old.size();
}

bool Block::isMutable() const noexcept {
    return mutable_;
}

int Block::insert(int pos, const Block& s) {
    return isMutable() 
        ? insertMutableImpl(pos, s) 
        : 0;
 }

int Block::remove(int pos, int count) {
    if (!isMutable()) {
        auto sz = size_;
        clearFromDelimeters();
        update("");
        return sz;
    }

    return removeMutableImpl(pos, count);
}

bool Block::merge(const Block& s) {
    if (!isMutable() || !s.isMutable())
        return false;
    auto sh = insert(end(), s);
    if (!sh)
        return false;
    format_flags_ = kFullFlags;
    return true;
}

void Block::update(const QString& s) {
    value_ = s;
    size_ = value_.size();
}

int Block::insertMutableImpl(int pos, const Block& s) {
    pos -= start_;
    auto sstr = s.toString(false);
    auto sz = sstr.size();
    auto delim = DelimetersContainer::find(pos);
    if (delim != delimeters_.end()) {
        if (delim->isInside(pos))
            pos = delim->pos;
        DelimetersContainer::shiftDelimeters(delim, sstr.size());
    }
    update(value_.insert(pos, std::move(sstr)));
    return sz;
}

int Block::removeMutableImpl(int pos, int count) {
    pos -= start_;
    auto total{ 0 }, delim_offset{ 0 };
    auto delim_it = DelimetersContainer::find(pos);
    while (pos < value_.size() && total < count) {
        if (delim_it != delimeters_.end() && delim_it->isInside(pos)) {
            auto sz = delim_it->size;
            total += sz;
            pos = delim_it->pos;
            delim_it = DelimetersContainer::removeDelimeter(delim_it, 0);
            continue;
        }
        auto bound = (delim_it != delimeters_.end()) ? delim_it->pos : value_.size();
        auto sz = std::min(bound - pos, count - total);
        value_.remove(pos, sz);
        DelimetersContainer::shiftDelimeters(delim_it, -sz);
        total += sz;
    }
    size_ = value_.size();
    return total;
}

QString Block::toString(bool delims) const {
    return delims 
        ? value_ 
        : removeDelimeters();
}

BlockPtr Block::clone() const {
    return BlockPtr(new Block(start_, type_, value_, mutable_, format_flags_, delimeters_));
}

Symbol::Symbol(
    int start, 
    const QString& value, 
    calculator::symbol_type symbolType, 
    format_t formatFlags, 
    const std::list<Delimeter>& delimeters) :
    Block(start, calculator::token_type::SYMBOL, value, true, formatFlags, delimeters),
    symbol_type_{ symbolType }
{ }

calculator::symbol_type Symbol::symbol_type() const noexcept {
    return symbol_type_;
}

int Symbol::refresh() {
    auto old_sz = size_;
    auto val = transformToTree(value_, -1);
    update(val);
    updateType(val);
    return size_ - old_sz;
}

BlockPtr Symbol::clone() const {
    return BlockPtr(new Symbol(start_, value_, symbol_type_, format_flags_, delimeters_));
}

int Symbol::insertMutableImpl(int pos, const Block& s) {
    if (s.type() != calculator::token_type::SYMBOL)
        return 0;

    auto bl = Block(start_, type_, value_);
    bl.insert(pos, s);

    auto old = toString(false);
    auto val = bl.toString(false);
    auto upd = transformToTree(val, -1);

    int res{ 0 };
    if (upd.size() > old.size()) {
        res = Block::insertMutableImpl(pos, s);
        updateType(toString(false));
    }

    return res;
}

int Symbol::removeMutableImpl(int pos, int count) {
    auto res = Block::removeMutableImpl(pos, count);
    updateType(value_);
    return res;
}

void Symbol::updateType(const QString& val) {
    auto token = calculator::lexer(value_.toStdWString(), Settings::max_output_size).get_token();
    if (token.type == calculator::token_type::SYMBOL)
        symbol_type_ = std::any_cast<calculator::symbol_type>(token.value);
    else
        symbol_type_ = calculator::symbol_type::UNKNOWN;
}

QString Symbol::transformToTree(const QString& val, int threshold) {
    auto [nval, st] = symbols_trie_.find_nearest_far(val.toStdWString());
    auto sz = static_cast<int>(nval.size());
    return (sz <= threshold) 
        ? val 
        : QString::fromStdWString(nval);
}

Number::Number(
    int start, 
    const QString &value, 
    format_t formatFlags, 
    const std::list<Delimeter>& delimeters) :
    Block(start, calculator::token_type::NUMBER, value, true, formatFlags, delimeters),
    calculated_number_{0}
{ }

Number::Number(
    int start, 
    const calculator::number_t& value, 
    format_t formatFlags, 
    const std::list<Delimeter>& delimeters) :
    Block(start, calculator::token_type::NUMBER, convertNumberToString(value), false, formatFlags, delimeters),
    calculated_number_{ value }
{ }

int Number::insertMutableImpl(int pos, const Block& s) {
    return (s.type() == calculator::token_type::NUMBER)
        ? Block::insertMutableImpl(pos, s)
        : 0;
}

QString Number::convertNumberToString(const calculator::number_t &num) const {
    return QString::fromStdWString(
        calculator::convert_to_wstring(num, Settings::max_output_size)
    );
}

calculator::number_t Number::get() const {
    return !isMutable()
        ? calculated_number_
        : calculator::number_t{ toString().toStdString() };
}

BlockPtr Number::clone() const {
    return isMutable()
        ? BlockPtr(new Number(start_, value_, format_flags_, delimeters_))
        : BlockPtr(new Number(start_, calculated_number_, format_flags_, delimeters_));
}

Space::Space(int start) : Block(start, calculator::token_type::EMPTY, " ", false)
{ }

LeftBracket::LeftBracket(int start) : Block(start, calculator::token_type::LBRACKET, "(", false)
{ }

RightBracket::RightBracket(int start) : Block(start, calculator::token_type::RBRACKET, ")", false)
{ }