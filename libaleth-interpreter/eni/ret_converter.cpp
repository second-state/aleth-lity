#include <sstream>
#include <string>
#include <iomanip>
#include <vector>


#include <boost/format.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "ret_converter.hpp"
#include "mphelpers.hpp"

using RetConverterError = std::runtime_error;

class RetConverter
{
    using typeInfo_t = std::vector<Code>;
    std::ostringstream out;
    const typeInfo_t& typeInfo;
    const std::string& json;
    typeInfo_t::const_iterator t;
    std::string::const_iterator j;

public:
    RetConverter(const std::vector<Code>& typeInfo, const std::string& json)
      : typeInfo(typeInfo), json(json), t(typeInfo.begin()), j(json.begin())
    {}
    std::string execute()
    {
        skipWS();
        expect('[');
        for (int i = 0; i + t < typeInfo.end(); ++i)
        {
            if (0 < i)
            {
                skipWS();
                expect(',');
            }
            parseType();
        }
        skipWS();
        expect(']');
        return out.str();
    }

private:
    void parseType()
    {
        if (IsComplexType(*t))
        {
            switch (*t)
            {
            case FIX_ARRAY_START:
                parseFixArray();
                break;
            case DYN_ARRAY_START:
                parseDynArray();
                break;
            case STRUCT_START:
                parseStruct();
                break;
            case STRING:
                parseString();
                break;
            default:
                assert(false);
            }
        }
        else
        {
            parseValue();
        }
    }
    void parseString()
    {
        ++t;
        skipWS();
        expect('"');
        int64_t length = 0;
        std::stringstream buf;
        while (*j != '"')
        {
            if (*j == '\\')
            {
                buf.put(parseEscape());
            }
            else
            {
                buf.put(*j);
                ++j;
            }
            ++length;
        }
        boost::multiprecision::uint256_t length256(length);
        out << dev::toBigEndianString(length256);
        out << buf.rdbuf();
        if (length % 32)
        {
            for (int i = length % 32; i < 32; ++i)
            {
                out.put(0);
            }
        }

        expect('"');
    }
    void parseDynArray() { throw RetConverterError("dynamic array not implemented yet!"); }
    void parseFixArray()
    {
        ++t;
        skipWS();
        expect('[');
        if (t + 32 > typeInfo.end()) {
            throw RetConverterError("typeInfo index out of range");
        }
        auto leng = (int64_t)s256FromBigEndian(t);
        t += 32;

        for (int64_t i = 0; i < leng; ++i)
        {
            if (i > 0)
            {
                skipWS();
                expect(',');
            }
            if (i == leng - 1)
            {
                parseType();
            }
            else
            {
                auto t0 = t;
                parseType();
                t = t0;
            }
        }

        skipWS();
        expect(']');
    }
    void parseStruct()
    {
        skipWS();
        expect('[');
        {
            skipWS();
            expect(',');
            if (*t != STRUCT_END)
            {
                parseType();
            }
        }
        ++t;
        skipWS();
        expect(']');
    }
    void parseValue()
    {
        skipWS();
        if (*t == BOOL)
        {
            if (have('t'))
            {
                expect('r');
                expect('u');
                expect('e');
                for (int i = 0; i < 31; ++i)
                {
                    out.put(0);
                }
                out.put(1);
            }
            else if (have('f'))
            {
                expect('a');
                expect('l');
                expect('s');
                expect('e');
                for (int i = 0; i < 32; ++i)
                {
                    out.put(0);
                }
            }
            else
            {
                throw RetConverterError(
                    boost::str(boost::format("expected boolean, found '%c'") % *j));
            }
        }
        else if (IsSint(*t))
        {
            auto j0 = j;
            have('-');
            while (haveDigit())
                ;
            if (j0 == j or (j - j0 == 1 and *j0 == '-'))
            {
                throw RetConverterError(
                    boost::str(boost::format("expected int, found '%c'") % *j0));
            }
            boost::multiprecision::int256_t value(std::string(j0, j));
            out << dev::toBigEndianString(dev::s2u(value));
        }
        else if (IsUint(*t))
        {
            auto j0 = j;
            while (haveDigit())
                ;
            if (j0 == j)
            {
                throw RetConverterError(
                    boost::str(boost::format("expected uint, found '%c'") % *j0));
            }
            boost::multiprecision::uint256_t value(std::string(j0, j));
            out << dev::toBigEndianString(value);
        }
        else
        {
            throw RetConverterError(boost::str(
                boost::format("endcoding error - unknown or not implemented type: %1$d") % *t));
        }
        ++t;
    }
    char parseEscape()
    {
        char ch;
        switch (j[1])
        {
        case '\\':
        case '"':
        case '/':
            ch = j[1];
            break;
        case 'b':
            ch = '\b';
            break;
        case 'f':
            ch = '\f';
            break;
        case 'n':
            ch = '\n';
            break;
        case 'r':
            ch = '\r';
            break;
        case 't':
            ch = '\t';
            break;
        case 'u':
        {
            std::string str(j + 2, j + 6);
            int code = std::stoi(str, 0, 16);
            if (code < 128)
            {
                ch = code;
            }
            else
            {
                throw RetConverterError("unicode not implemented yet!");
            }
            break;
        }
        default:
            throw RetConverterError("invalid esscape sequence");
        }
        if (j[1] == 'u')
        {
            j += 6;
        }
        else
        {
            j += 2;
        }
        return ch;
    }
    bool have(char c)
    {
        if (*j == c)
        {
            ++j;
            return true;
        }
        return false;
    }
    bool haveDigit()
    {
        if (j == json.end())
        {
            return false;
        }
        if ('0' <= *j and *j <= '9')
        {
            ++j;
            return true;
        }
        return false;
    }
    void expect(char c)
    {
        if (*j != c)
        {
            throw RetConverterError(
                boost::str(boost::format("expected '%1$c', found '%2$c'") % c % (*j)));
        }
        else
        {
            ++j;
        }
    }
    void skipWS()
    {
        while (j != json.end())
        {
            switch (*j)
            {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                ++j;
                continue;
            }
            break;
        }
    }
};

std::string ConvertReturnValue(const std::vector<Code>& typeInfo, const std::string& json)
{
    RetConverter cvt(typeInfo, json);
    return cvt.execute();
}
