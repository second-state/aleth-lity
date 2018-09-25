#pragma once

#include <string>

template <class Number, class Iterator>
void import_big_endian(Number& n, Iterator it)
{
    for (int i = 0; i < 32; ++i, ++it)
    {
        n <<= 8;
        n |= *it;
    }
}

template <class T>
std::string export_big_endian_ostream(T value)
{
    std::string str(32, 0);
    for (int i = 0; i < 32; ++i)
    {
        str[31 - i] = (unsigned)(value & 0xff);
        value >>= 8;
    }
    return str;
}
