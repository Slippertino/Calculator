#pragma once

#include <iterator>

namespace std {
    template<class BidirIt>
    constexpr BidirIt prev_or_default(BidirIt it, 
        BidirIt begin,          BidirIt end,
        BidirIt on_begin,       BidirIt on_end,
        typename std::iterator_traits<BidirIt>::difference_type n = 1)
    {
        if (it == end)
            return on_end;

        if (std::distance(begin, it) < n)
            return on_begin;

        std::advance(it, -n);
        return it;
    }

    template<class BidirIt>
    constexpr BidirIt prev_or_default(BidirIt it,
        BidirIt begin, BidirIt end, BidirIt on_default,
        typename std::iterator_traits<BidirIt>::difference_type n = 1)
    {
        return prev_or_default(it, begin, end, on_default, on_default, n);
    }

    template<class BidirIt>
    constexpr BidirIt next_or_default(BidirIt it, 
        BidirIt end, BidirIt on_end,
        typename std::iterator_traits<BidirIt>::difference_type n = 1)
    {
        if (it == end || std::distance(it, end) < n)
            return on_end;

        std::advance(it, n);
        return it;
    }

    template<class BidirIt>
    constexpr BidirIt next_or_default(
        BidirIt it,
        BidirIt end, 
        typename std::iterator_traits<BidirIt>::difference_type n = 1)
    {
        return next_or_default(it, end, end, n);
    }
}