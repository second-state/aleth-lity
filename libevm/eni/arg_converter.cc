#include <sstream>
#include <string>
#include <vector>

#include <boost/format.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "codes.hh"
#include "mphelpers.hh"

using ArgumentParserError = std::runtime_error;


class ArgConverter
{
    using typeInfo_t = std::vector<Code>;
    std::ostringstream out;
    const typeInfo_t& typeInfo;
    const std::string& data;
    typeInfo_t::const_iterator ti;
    std::string::const_iterator di;
    void parseType()
    {
        auto t = *ti;
        if (IsComplexType(t))
        {
            if (t == FIX_ARRAY_START)
            {
                parseFixArray();
            }
            else if (t == DYN_ARRAY_START)
            {
                parseDynArray();
            }
            else if (t == STRUCT_START)
            {
                parseStruct();
            }
            else if (t == STRING)
            {
                parseString();
            }
        }
        else
        {
            parseValue();
        }
    }
    void parseString()
    {
        ti++;
        boost::multiprecision::int256_t leng256;
        import_big_endian(leng256, di);
        int64_t leng = static_cast<int64_t>(leng256);
        di += 32;

        std::stringstream buffer;
        for (int64_t i = 0; i < leng; i++)
        {
            if (di[i] == '\\' || di[i] == '"')
            {
                buffer << '\\' << di[i];
            }
            else if ((uint8_t)di[i] < 0x20)
            {  // control characters
                buffer << boost::format("\\u%04x") % (unsigned)di[i];
            }
            else
            {
                buffer << di[i];
            }
        }
        out << "\"";
        out << buffer.rdbuf();
        out << "\"";
        di += leng;
        if (leng % 32)
        {
            di += 32 - leng % 32;
        }
    }
    void parseFixArray()
    {
        ti++;
        out << "[";
        boost::multiprecision::int256_t leng256;
        import_big_endian(leng256, ti);
        int64_t leng = static_cast<int64_t>(leng256);
        ti += 32;

        for (int64_t i = 0; i < leng; i++)
        {
            if (i == leng - 1)
            {
                parseType();
            }
            else
            {
                out << ", ";
                auto preserveTi = ti;
                parseType();
                ti = preserveTi;
            }
        }

        out << "]";
    }
    void parseDynArray() { throw ArgumentParserError("dynamic array not implemented yet!"); }
    void parseStruct()
    {
        ti++;  // struct_start
        out << "[";
        for (bool i = false; ti < typeInfo.end(); i = true)
        {
            auto t = *ti;
            if (i)
            {
                out << ", ";
            }
            if (t != STRUCT_END)
            {
                parseType();
            }
            else
            {
                break;
            }
        }
        if (*ti != STRUCT_END)
        {
            throw ArgumentParserError("encoding error - expected struct_end token");
        }
        ti++;
        out << "]";
    }
    void parseValue()
    {
        auto t = *ti;
        if (t == BOOL)
        {
            bool boolVal = std::any_of(di, di + 32, [](char i) -> bool { return i; });
            if (boolVal)
            {
                out << "true";
            }
            else
            {
                out << "false";
            }
        }
        else if (IsSint(t))
        {
            boost::multiprecision::int256_t n;
            import_big_endian(n, di);
            out << n;
        }
        else if (IsUint(t))
        {
            boost::multiprecision::uint256_t n;
            import_big_endian(n, di);
            out << n;
        }
        else
        {
            throw ArgumentParserError(
                boost::str(boost::format("unknown or not implemented type %1$d") % t));
        }
        ti++;
        di += 32;
    }

public:
    ArgConverter(const std::vector<Code>& typeInfo, const std::string& data)
      : out(), typeInfo(typeInfo), data(data), ti(typeInfo.begin()), di(data.begin())
    {}
    std::string execute()
    {
        out << "[";
        for (bool i = false; ti < typeInfo.end(); i = true)
        {
            if (i)
            {
                out << ",";
            }
            parseType();
        }
        out << "]";
        return out.str();
    }
};

std::string ConvertArguments(const std::vector<Code>& typeInfo, const std::string& data)
{
    ArgConverter cvt(typeInfo, data);
    return cvt.execute();
}
