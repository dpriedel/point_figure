// =====================================================================================
//
//       Filename:  DDecQuad.cpp
//
//    Description:  implementation for DDecQuad 
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
#include <string_view>

#include "DDecQuad.h"
#include "decContext.h"


using namespace DprDecimal;

decContext DDecQuad::mCtx_ ;
bool DDecQuad::context_initialized_ = false;

void DDecQuad::InitContext()
{
    if (!DDecQuad::context_initialized_)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        decContextSetRounding(&DDecQuad::mCtx_, DEC_ROUND_HALF_UP);
        DDecQuad::context_initialized_ = true;
    }
}

DDecQuad::DDecQuad()
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
}

DDecQuad::DDecQuad(const DDecQuad& rhs)
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadCopy(&this->decimal_, &rhs.decimal_);
}

DDecQuad::DDecQuad(DDecQuad&& rhs) noexcept
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadCopy(&this->decimal_, &rhs.decimal_);
    decQuadZero(&rhs.decimal_);
}

DDecQuad::DDecQuad(const char* number)
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadFromString(&this->decimal_, number, &DDecQuad::mCtx_);
}

DDecQuad::DDecQuad(const std::string& number)
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadFromString(&this->decimal_, number.data(), &DDecQuad::mCtx_);
}

DDecQuad::DDecQuad(std::string_view number)
    : DDecQuad{std::string{number}}
{
}

DDecQuad::DDecQuad(int32_t number)
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadFromInt32(&this->decimal_, number);
}

DDecQuad::DDecQuad(uint32_t number)
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadFromUInt32(&this->decimal_, number);
}

DDecQuad::DDecQuad(const decNumber& number)
    : decimal_{}
{
    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadFromNumber(&this->decimal_, &number, &DDecQuad::mCtx_);
}

DDecQuad::DDecQuad(double number)
    : decimal_{}
{
    std::array<char, 30> buf{};
    if (auto [p, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), number, std::chars_format::fixed);
            ec != std::errc())
    {
        throw std::runtime_error(fmt::format("Problem converting double to decimal: {}\n", std::make_error_code(ec).message()));
    }
    else
    {
        // string is NOT NULL terminated
        *p = '\0';
    }

    DDecQuad::InitContext();
    decQuadZero(&this->decimal_);
    decQuadFromString(&this->decimal_, buf.data(), &DDecQuad::mCtx_);
}

DDecQuad& DDecQuad::operator=(const DDecQuad& rhs)
{
    if (this != &rhs)
    {
        decQuadCopy(&this->decimal_, &rhs.decimal_);
    }
    return *this;
}

DDecQuad& DDecQuad::operator=(DDecQuad&& rhs) noexcept
{
    if (this != &rhs)
    {
        decQuadCopy(&this->decimal_, &rhs.decimal_);
        decQuadZero(&rhs.decimal_);
    }
    return *this;
}

DDecQuad& DDecQuad::operator=(int32_t rhs)
{
    DDecQuad tmp{rhs};
    decQuadCopy(&this->decimal_, &tmp.decimal_);
    return *this;
}

DDecQuad& DDecQuad::operator=(uint32_t rhs)
{
    DDecQuad tmp{rhs};
    decQuadCopy(&this->decimal_, &tmp.decimal_);
    return *this;
}

DDecQuad& DDecQuad::operator=(double rhs)
{
    DDecQuad tmp{rhs};
    decQuadCopy(&this->decimal_, &tmp.decimal_);
    return *this;
}

DDecQuad& DDecQuad::operator=(const char* rhs)
{
    DDecQuad tmp{rhs};
    decQuadCopy(&this->decimal_, &tmp.decimal_);
    return *this;
}

DDecQuad& DDecQuad::operator=(const std::string& rhs)
{
    DDecQuad tmp{rhs};
    decQuadCopy(&this->decimal_, &tmp.decimal_);
    return *this;
}

DDecQuad& DDecQuad::operator=(const decNumber& rhs)
{
    decQuadFromNumber(&this->decimal_, &rhs, &DDecQuad::mCtx_);
    return *this;
}

int32_t DDecQuad::ToIntRounded () const
{
    int32_t result = decQuadToInt32(&decimal_, &DDecQuad::mCtx_, DEC_ROUND_HALF_DOWN);
    return result;
}		// -----  end of method DDecQuad::ToIntRounded  ----- 

int32_t DDecQuad::ToIntTruncated () const
{
    int32_t result = decQuadToInt32(&decimal_, &DDecQuad::mCtx_, DEC_ROUND_DOWN);
    return result;
}		// -----  end of method DDecQuad::ToIntTruncated  ----- 


double DDecQuad::ToDouble () const
{
    // I don't see a better way to do this.

    std::string temp = this->ToStr();
    double result{};
    if (auto [p, ec] = std::from_chars(temp.data(), temp.data() + temp.size(), result); ec != std::errc())
    {
        throw std::runtime_error(fmt::format("Problem converting decimal to double: {}\n", std::make_error_code(ec).message()));
    }
    return result ;
}		// -----  end of method DDecQuad::ToDouble  ----- 

DDecQuad DDecQuad::abs() const
{
    DDecQuad result;
    decQuadAbs(&result.decimal_, &this->decimal_, &DDecQuad::mCtx_);

    return result;
}		// -----  end of method DDecQuad::abs  ----- 

//DDecQuad& DDecQuad::Rescale(std::string_view decimal_digits)
//{
////    char output [DECQUAD_String];
////    decQuadToString(&this->decimal_, output);
//    decNumber temp;
//    decNumberZero(&temp);
//    decQuadToNumber(&this->decimal_, &temp);
//
////    char output1 [DECQUAD_String];
////    decNumberToString(&temp, output1);
//    decNumber digits;
//    decNumberFromString(&digits, decimal_digits.data(), &DDecQuad::mCtx_);
//
//    decNumber temp2;
//    decNumberZero(&temp2);
//    decNumberQuantize(&temp2, &temp, &digits, &DDecQuad::mCtx_);
//
//    decQuadFromNumber(&this->decimal_, &temp2, &DDecQuad::mCtx_);
//    return *this;
//}

DDecQuad& DDecQuad::Rescale(int32_t exponent)
{
//    char output [DECQUAD_String];
//    decQuadToString(&this->decimal_, output);
    decNumber temp;
    decNumberZero(&temp);
    decQuadToNumber(&this->decimal_, &temp);

//    char output1 [DECQUAD_String];
//    decNumberToString(&temp, output1);
    decNumber digits;
    decNumberFromInt32(&digits, exponent);

    decNumber temp2;
    decNumberZero(&temp2);
    decNumberRescale(&temp2, &temp, &digits, &DDecQuad::mCtx_);

    decQuadFromNumber(&this->decimal_, &temp2, &DDecQuad::mCtx_);
    return *this;
}

DDecQuad DDecQuad::log_n() const
{
    decNumber temp;
    decQuadToNumber(&this->decimal_, &temp);
    decNumber ln_temp;
    decNumberLn(&ln_temp, &temp, &DDecQuad::mCtx_);
    DDecQuad result;
    decQuadFromNumber(&result.decimal_, &ln_temp, &DDecQuad::mCtx_);
    return result;
}		// -----  end of method DDecQuad::log_n  ----- 

DDecQuad DDecQuad::exp_n() const
{
    decNumber temp;
    decQuadToNumber(&this->decimal_, &temp);
    decNumber exp_temp;
    decNumberExp(&exp_temp, &temp, &DDecQuad::mCtx_);
    DDecQuad result;
    decQuadFromNumber(&result.decimal_, &exp_temp, &DDecQuad::mCtx_);
    return result;
}		// -----  end of method DDecQuad::exp_n  ----- 

DDecQuad DDecQuad::ToPower (const DDecQuad& power) const
{
    decNumber temp;
    decQuadToNumber(&this->decimal_, &temp);
    decNumber power_temp;
    decQuadToNumber(&power.decimal_, &power_temp);
    decNumber result_temp;
    decNumberPower(&result_temp, &temp, &power_temp, &DDecQuad::mCtx_);
    DDecQuad result;
    decQuadFromNumber(&result.decimal_, &result_temp, &DDecQuad::mCtx_);
    return result;
}		// -----  end of method DDecQuad::ToPower  ----- 

// ===  FUNCTION  ======================================================================
//         Name:  max
//  Description:  
// =====================================================================================
    
DDecQuad DprDecimal::max(const DDecQuad& lhs, const DDecQuad& rhs)
{
    DDecQuad result;
    decQuadMax(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
    return result;
}		// -----  end of function max  -----

