#pragma once
#include <iterator>
#include <optional>

#include "hlc.hpp"

/* Select iterator to the element whose HLC is the greatest (LWW).
   – `It` must dereference to a type that has `.clock`  */
template <typename It>
[[nodiscard]] It lww_select(It first, It last) {
    if (first == last) return last;
    It best = first;
    for (++first; first != last; ++first)
        if (best->clock < first->clock) best = first;
    return best;
}

/* Convenience wrapper for containers that expose `begin()`/`end()` */
template <typename Container>
[[nodiscard]] auto lww_select(Container &&c)
    -> std::optional<typename Container::value_type> {
    auto it = lww_select(std::begin(c), std::end(c));
    if (it == std::end(c)) return std::nullopt;
    return *it;
}
