// =====================================================================================
//
//       Filename:  DDecDouble.cpp
//
//    Description:  implementation for DDecDouble 
//                  provides a place to allocate static context member and
//                  initialize it.
//        Version:  1.0
//        Created:  07/22/2021 09:19:02 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#include <array>
#include <charconv>

#include <fmt/format.h>

#include "DDecDouble.h"
#include "decContext.h"

using namespace DprDecimal;

decContext DDecDouble::mCtx ;

DDecDouble::DDecDouble()
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
}

DDecDouble::DDecDouble(const char* number)
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
    decDoubleFromString(&this->decimal_, number, &DDecDouble::mCtx);
}

DDecDouble::DDecDouble(const std::string& number)
    : DDecDouble(number.c_str())	{ }

DDecDouble::DDecDouble(int32_t number)
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
    decDoubleFromInt32(&this->decimal_, number);
}

DDecDouble::DDecDouble(uint32_t number)
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
    decDoubleFromUInt32(&this->decimal_, number);
}

DDecDouble::DDecDouble(double number, int dec_digits)
{
    std::array<char, 30> buf{};
    if (auto [p, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), number, std::chars_format::fixed, dec_digits);
            ec != std::errc())
    {
        throw std::runtime_error(fmt::format("Problem converting double to decimal: {}\n", std::make_error_code(ec).message()));
    }
    else
    {
        // string is NOT NULL terminated
        buf[buf.size()] = '\0';
    }

//    std::ostringstream temp;
//    temp << std::fixed << std::setprecision(dec_digits) << number;

    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
//    decDoubleFromString(&this->decimal_, temp.str().c_str(), &DDecDouble::mCtx);
    decDoubleFromString(&this->decimal_, buf.data(), &DDecDouble::mCtx);
//		decDoubleReduce(&this->decimal_, &this->decimal_, &DDecDouble::mCtx);
}

int32_t DDecDouble::ToIntRounded () const
{
    int32_t result = decDoubleToInt32(&decimal_, &DDecDouble::mCtx, DEC_ROUND_HALF_DOWN);
    return result;
}		// -----  end of method DDecDouble::ToIntRounded  ----- 

int32_t DDecDouble::ToIntTruncated () const
{
    int32_t result = decDoubleToInt32(&decimal_, &DDecDouble::mCtx, DEC_ROUND_DOWN);
    return result;
}		// -----  end of method DDecDouble::ToIntTruncated  ----- 

