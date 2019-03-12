#pragma once

#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/range/iterator_range_core.hpp>

#include "../../libdevcore/CommonData.h"

template <class Iterator>
boost::multiprecision::uint256_t u256FromBigEndian(Iterator it) {
    return dev::fromBigEndian<dev::u256>(boost::make_iterator_range_n(it, 32));
}

template <class Iterator>
boost::multiprecision::int256_t s256FromBigEndian(Iterator it) {
    return dev::u2s(u256FromBigEndian(it));
}
