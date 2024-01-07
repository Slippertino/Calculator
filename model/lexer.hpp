#pragma once
#pragma warning(disable: 4244)

#include <any>
#include <string>
#include <functional>
#include <unordered_map>
#include "status.hpp"
#include "number.hpp"
#include "token.hpp"

namespace calculator {

class lexer {
public:
    lexer() = delete;

    lexer(std::wistream &istr, int prec);
    lexer(const std::wstring& str, int prec);

    bool empty() const noexcept;
    std::wstring get_last() const;
    int get_current_position() const;

    token_t get_token();

    static std::vector<std::wstring> get_symbols();

private:
    void get_char();
    void unget_char() noexcept;

    bool is_digit() const noexcept;
    bool is_whitespace() const noexcept;
    void skip_whites();

    void read_until_bound(std::wostringstream &out, const std::function<bool()> &pred);

    token_t get_number();
    token_t get_symbol();

private:
    inline static const std::unordered_map<std::wstring, symbol_type> symbols_ = {
        { L"+",      symbol_type::ADD            },
        { L"x",      symbol_type::MULT           },
        { L"/",      symbol_type::DIV            },
        { L"^",      symbol_type::POW            },
        { L"-",      symbol_type::MINUS          },
        { L"!",      symbol_type::FACT           },
        { L"%",      symbol_type::MOD            },
        { L"cos",    symbol_type::COS            },
        { L"sin",    symbol_type::SIN            },
        { L"tan",    symbol_type::TAN            },
        { L"asin",   symbol_type::ASIN           },
        { L"acos",   symbol_type::ACOS           },
        { L"atan",   symbol_type::ATAN           },
        { L"lg",     symbol_type::LG             },
        { L"ln",     symbol_type::LN             },

        { L"sqrt",   symbol_type::SQRT           },
        { L"\u221A", symbol_type::SQRT           },

        { L"PI",      symbol_type::PI            },
        { L"\u03C0",  symbol_type::PI            },

        { L"E",      symbol_type::E              },
        { L"\u03B5", symbol_type::E              },
    };

private:
    bool empty_{false}, blocked_{false};
    wchar_t cur_;
    int precision_;

    std::wstring last_;
    int pos_{ 0 };
    std::wistringstream source_;
    std::wistream &stream_;
};

} // namespace calculator
