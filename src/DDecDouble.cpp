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

decContext DprDecimal::DDecDouble::mCtx ;

DprDecimal::DDecDouble::DDecDouble()
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
}

DprDecimal::DDecDouble::DDecDouble(const char* number)
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
    decDoubleFromString(&this->mDecimal, number, &DDecDouble::mCtx);
}

DprDecimal::DDecDouble::DDecDouble(const std::string& number)
    : DDecDouble(number.c_str())	{ }

DprDecimal::DDecDouble::DDecDouble(int32_t number)
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
    decDoubleFromInt32(&this->mDecimal, number);
}

DprDecimal::DDecDouble::DDecDouble(uint32_t number)
{
    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
    decDoubleFromUInt32(&this->mDecimal, number);
}

DprDecimal::DDecDouble::DDecDouble(double number, int dec_digits)
{
    //    once GCC fully supports from_chars, use the code below
//    std::array<char, 30> buf;
//    if (auto [p, ec] = std::from_chars(buf.data(), buf.data() + buf.size(), number, std::chars_format::fixed, dec_digits);
//            ec != std::errc())
//    {
//        throw std::runtime_error(fmt::format("Problem converting double to decimal: {}\n", std::make_error_code(ec).message()));
//    }
//    else
//    {
//        // string is NOT NULL terminated
//        buf[p] = '\0';
//    }

    std::ostringstream temp;
    temp << std::fixed << std::setprecision(dec_digits) << number;

    decContextDefault(&DDecDouble::mCtx, DEC_INIT_DECDOUBLE);
    decDoubleFromString(&this->mDecimal, temp.str().c_str(), &DDecDouble::mCtx);
//		decDoubleReduce(&this->mDecimal, &this->mDecimal, &DDecDouble::mCtx);
}
