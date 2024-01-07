#include <sstream>
#include "lexer.hpp"

namespace calculator {

token_t result(token_type type, status_type status, std::any val = nullptr){
    return token_t{ type, status, val };
}

token_t ok(token_type type, std::any val = nullptr) {
    return result(type, status_type::OK, val);
}

lexer::lexer(std::wistream& istr, int prec) : 
    cur_{ L' ' }, 
    precision_{ prec }, 
    stream_ { istr }
{ }

lexer::lexer(const std::wstring& str, int prec) : 
    cur_{ L' ' },
    precision_{ prec },
    source_{ str },
    stream_{ source_ }
{ }

bool lexer::empty() const noexcept {
    return empty_;
}

std::wstring lexer::get_last() const {
    return last_;
}

int lexer::get_current_position() const {
    return pos_;
}

token_t lexer::get_token() {
    skip_whites();

    if (empty_) {
        last_.clear();
        return empty_token;
    }

    switch (cur_)
    {
    case L'(':
        last_ = cur_;
        get_char();
        return ok(token_type::LBRACKET);
    case L')':
        last_ = cur_;
        get_char();
        return ok(token_type::RBRACKET);
    default:
        if (is_digit())
            return get_number();
        else
            return get_symbol();
    }
}

std::vector<std::wstring> lexer::get_symbols() {
    std::vector<std::wstring> res;
    res.reserve(symbols_.size());
    std::transform(
        symbols_.begin(), symbols_.end(), 
        std::back_inserter(res), [](auto&& val) { return val.first; }
    );
    return res;
}

void lexer::get_char() {
    if (empty_)
        return;

    if (blocked_) {
        blocked_ = false;
        ++pos_;
    }
    else {
        if (!(stream_ >> std::noskipws >> cur_))
            empty_ = true;
        else
            ++pos_;
    }
}

void lexer::unget_char() noexcept {
    blocked_ = true;
    --pos_;
}

bool lexer::is_digit() const noexcept {
    return iswdigit(cur_) || cur_ == L'.';
}

bool lexer::is_whitespace() const noexcept {
    static const std::wstring whites = L" \n\t";
    return whites.find(cur_) != std::wstring::npos;
}

void lexer::skip_whites() {
    do { 
        get_char(); 
    } while(!empty_ && is_whitespace());
    
    if (!empty_)
        unget_char();
}

void lexer::read_until_bound(std::wostringstream& out, const std::function<bool()>& pred) {
    get_char();
    while (!empty_ && pred()) {
        out << cur_;
        get_char();
    }
    if (!empty_)
        unget_char();
}

token_t lexer::get_number() {
    std::wostringstream num_ostr;
    wchar_t prev{0};
    int digs_count{ 0 };
    bool exp_found{false}, leading_zeros_end{false};
    read_until_bound(num_ostr, [&]() {
        auto isdig = static_cast<bool>(is_digit());
        auto isexp = towlower(cur_) == L'e';
        auto isreal = isexp || cur_  == L'.';
        auto exp_part = (cur_ == L'+' || cur_ == L'-' || isdig) && exp_found;

        leading_zeros_end |= (isdig && cur_  != L'0');
        exp_found |= isexp;
        digs_count += !exp_found && leading_zeros_end && isdig;

        prev = cur_;
        return isdig || isreal || exp_part;
    });

    try {
        last_ = num_ostr.str();
        if (digs_count > precision_)
            return result(token_type::NUMBER, status_type::TOO_LONG_NUMBER);
        number_t num;
        num.set_prec(4 * precision_);
        num = std::string{ last_.begin(), last_.end() };
        return ok(token_type::NUMBER, num);
    }
    catch(std::invalid_argument){
        return result(token_type::NUMBER, status_type::INVALID_NUMBER);
    }
    catch(...){
        return result(token_type::NUMBER, status_type::UNKNOWN_ERROR);
    }
}

token_t lexer::get_symbol() {
    std::wostringstream op_ostr;
    read_until_bound(op_ostr, [&]() {
        return !is_digit() && symbols_.find(op_ostr.str()) == symbols_.end();
    });

    last_ = op_ostr.str();

    return symbols_.find(last_) != symbols_.end()
        ? ok(token_type::SYMBOL, symbols_.at(last_))
        : result(token_type::SYMBOL, status_type::UNKNOWN_SYMBOL, symbol_type::UNKNOWN);
}

} // namespace calculator
