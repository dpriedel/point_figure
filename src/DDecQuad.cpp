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
//extern "C"
//{
//    #include <decContext.h>
//}


using namespace DprDecimal;

decContext DDecQuad::mCtx_ ;
bool DDecQuad::context_initialized_ = false;


DDecQuad::DDecQuad()
{
    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
}

DDecQuad::DDecQuad(const DDecQuad& rhs)
{
    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
    decQuadCopy(&this->decimal_, &rhs.decimal_);
}

DDecQuad::DDecQuad(DDecQuad&& rhs)
{
    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
    decQuadCopy(&this->decimal_, &rhs.decimal_);
    decQuadZero(&rhs.decimal_);
}

//DDecQuad::DDecQuad(const char* number)
//{
//    decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
//    decQuadZero(&this->decimal_);
//    decQuadFromString(&this->decimal_, number, &DDecQuad::mCtx_);
//}

DDecQuad::DDecQuad(std::string_view number)
{
    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
    decQuadFromString(&this->decimal_, number.data(), &DDecQuad::mCtx_);
}

DDecQuad::DDecQuad(int32_t number)
{
    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
    decQuadFromInt32(&this->decimal_, number);
}

DDecQuad::DDecQuad(uint32_t number)
{
    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
    decQuadFromUInt32(&this->decimal_, number);
}

DDecQuad::DDecQuad(const decNumber& number)
{
    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
    decQuadFromNumber(&this->decimal_, &number, &DDecQuad::mCtx_);
}

DDecQuad::DDecQuad(double number, int dec_digits)
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

    if (context_initialized_ == false)
    {
        decContextDefault(&DDecQuad::mCtx_, DEC_INIT_DECQUAD);
        context_initialized_ = true;
    }
    decQuadZero(&this->decimal_);
//    decQuadFromString(&this->decimal_, temp.str().c_str(), &DDecQuad::mCtx_);
    decQuadFromString(&this->decimal_, buf.data(), &DDecQuad::mCtx_);
//		decQuadReduce(&this->decimal_, &this->decimal_, &DDecQuad::mCtx_);
}

DDecQuad& DDecQuad::operator=(const DDecQuad& rhs)
{
    if (this != &rhs)
    {
        decQuadCopy(&this->decimal_, &rhs.decimal_);
    }
    return *this;
}

DDecQuad& DDecQuad::operator=(DDecQuad&& rhs)
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

DDecQuad& DDecQuad::operator=(std::string_view rhs)
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

DDecQuad& DDecQuad::Rescale(std::string_view decimal_digits)
{
    char output [DECQUAD_String];
    decQuadToString(&this->decimal_, output);
    std::cout << "output: " << output << '\n';
    decNumber temp;
    decNumberZero(&temp);
    decQuadToNumber(&this->decimal_, &temp);

    char output1 [DECQUAD_String];
    decNumberToString(&temp, output1);
    std::cout << "output1: " << output1 << '\n';
    decNumber digits;
    decNumberFromString(&digits, decimal_digits.data(), &DDecQuad::mCtx_);

    decNumber temp2;
    decNumberZero(&temp2);
    decNumberQuantize(&temp2, &temp, &digits, &DDecQuad::mCtx_);

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

