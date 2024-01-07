#pragma once

#include <sstream>
#include "mpreal/mpreal.h"

namespace calculator {

using number_t = mpfr::mpreal;

inline std::wstring convert_to_wstring(const number_t &num, int prec) {
    auto n = num;
    if (!mpfr::isint(n)) {
        auto mult{ mpfr::pow(10, prec) };
        n = mpfr::round(n * mult) / mult;
        auto nabs = mpfr::abs(n);
        if (nabs == 0)
            n = nabs;
    }

    std::ostringstream ostr;
    ostr.precision(prec);
    ostr << n;
    auto str = ostr.str();
    return std::wstring{ str.begin(), str.end() };
}

} // namespace calculator
