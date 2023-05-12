
// =====================================================================================
//        Class:  DDecQuad
//  Description:  Dave's decimal class. wraps libDecnum. Provides only minimal
//				  code to do what I need right now.
// =====================================================================================

#ifndef _DDECQUAD_
#define _DDECQUAD_

#include <format>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string_view>

extern "C"
{
    #include <decQuad.h>
    #include <decimal128.h>
}

namespace DprDecimal
{
class DDecQuad;

    DDecQuad max(const DDecQuad& lhs, const DDecQuad& rhs);

//	example specialization


// =====================================================================================
//        Class:  DDecQuad
//  Description:  64 digit decimal class
// =====================================================================================

class DDecQuad
{
	friend DDecQuad operator+(const DDecQuad& lhs, const DDecQuad& rhs);
	friend DDecQuad operator-(const DDecQuad& lhs, const DDecQuad& rhs);
	friend DDecQuad operator*(const DDecQuad& lhs, const DDecQuad& rhs);
	friend DDecQuad operator/(const DDecQuad& lhs, const DDecQuad& rhs);

    friend DDecQuad Mod(const DDecQuad&lhs, const DDecQuad& rhs);

    // the 2 specializations for double internally round the
    // double to same number of decimals as the decNumber.

//	friend bool operator==(const DDecQuad& lhs, double rhs);
//	friend bool operator==(double lhs, const DDecQuad& rhs);

	friend bool operator==(const DDecQuad& lhs, const DDecQuad& rhs);
	friend bool operator!=(const DDecQuad& lhs, const DDecQuad& rhs);

	friend bool operator<(const DDecQuad& lhs, const DDecQuad& rhs);
	friend bool operator>(const DDecQuad& lhs, const DDecQuad& rhs);

	friend bool operator<=(const DDecQuad& lhs, const DDecQuad& rhs);
	friend bool operator>=(const DDecQuad& lhs, const DDecQuad& rhs);

    friend DDecQuad max(const DDecQuad& lhs, const DDecQuad& rhs);

	friend std::istream& operator>>(std::istream& is, DDecQuad& item);
	friend std::ostream& operator<<(std::ostream& os, const DDecQuad& item);

public:
    // ====================  LIFECYCLE     =======================================
    DDecQuad ();                             // constructor
    DDecQuad (const DDecQuad& rhs);
    DDecQuad (DDecQuad&& rhs) noexcept ;

    DDecQuad (const char* number);              // NOLINT(google-explicit-constructor)
    DDecQuad (const std::string& number);       // NOLINT(google-explicit-constructor)
    DDecQuad (std::string_view number);         // NOLINT(google-explicit-constructor)
    DDecQuad (int32_t number);                  // NOLINT(google-explicit-constructor)
    DDecQuad (uint32_t number);                 // NOLINT(google-explicit-constructor)
    DDecQuad (const decNumber& number);         // NOLINT(google-explicit-constructor)
    DDecQuad (double number);	                // NOLINT(google-explicit-constructor)

    ~DDecQuad() = default;

    // ====================  ACCESSORS     =======================================
    
    [[nodiscard]] std::string ToStr() const;
    [[nodiscard]] int32_t ToIntRounded() const;
    [[nodiscard]] int32_t ToIntTruncated() const;
    [[nodiscard]] double ToDouble() const;

    [[nodiscard]] int32_t GetExponent() const { return decQuadGetExponent(&decimal_); }

    [[nodiscard]] DDecQuad abs() const;
    [[nodiscard]] DDecQuad log_n() const;
    [[nodiscard]] DDecQuad exp_n() const;
    [[nodiscard]] DDecQuad ToPower(const DDecQuad& the_power) const;

    // ====================  MUTATORS      =======================================

    DDecQuad& Rescale(int32_t exponent);

    // ====================  OPERATORS     =======================================
    
    DDecQuad& operator+=(const DDecQuad& rhs);
    DDecQuad& operator-=(const DDecQuad& rhs);
    DDecQuad& operator*=(const DDecQuad& rhs);
    DDecQuad& operator/=(const DDecQuad& rhs);

    DDecQuad& operator=(const DDecQuad& rhs);
    DDecQuad& operator=(DDecQuad&& rhs) noexcept ;
    DDecQuad& operator=(int32_t rhs);
    DDecQuad& operator=(uint32_t rhs);
    DDecQuad& operator=(double rhs);
    DDecQuad& operator=(const decNumber& rhs);
    DDecQuad& operator=(const char* rhs);
    DDecQuad& operator=(const std::string& rhs);


protected:
    // ====================  DATA MEMBERS  =======================================

private:

    static void InitContext();

    // ====================  DATA MEMBERS  =======================================
    
    decQuad decimal_;
    static decContext mCtx_;

    static bool context_initialized_;

}; // -----  end of template class DDecimalSMALLDEC  -----

//
//	constructions/implicit conversion operators
//
	
inline std::string DDecQuad::ToStr() const
{
    char output [DECQUAD_String];
    decQuadToString(&this->decimal_, output);
    return {output};
}

//
//	member arithmetic operators
//

inline DDecQuad& DDecQuad::operator+=(const DDecQuad& rhs)
{
	decQuadAdd(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return *this;
}

inline DDecQuad& DDecQuad::operator-=(const DDecQuad& rhs)
{
	decQuadSubtract(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return *this;
}

inline DDecQuad& DDecQuad::operator*=(const DDecQuad& rhs)
{
	decQuadMultiply(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return *this;
}

inline DDecQuad& DDecQuad::operator/=(const DDecQuad& rhs)
{
	decQuadDivide(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return *this;
}

//
//	non-member arithmetic operators
//

inline DDecQuad operator+(const DDecQuad&lhs, const DDecQuad& rhs)
{
	DDecQuad result;
	decQuadAdd(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return result;
}

inline DDecQuad operator-(const DDecQuad&lhs, const DDecQuad& rhs)
{
	DDecQuad result;
	decQuadSubtract(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return result;
}

inline DDecQuad operator*(const DDecQuad&lhs, const DDecQuad& rhs)
{
	DDecQuad result;
	decQuadMultiply(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return result;
}

inline DDecQuad operator/(const DDecQuad&lhs, const DDecQuad& rhs)
{
	DDecQuad result;
	decQuadDivide(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return result;
}

inline DDecQuad Mod(const DDecQuad&lhs, const DDecQuad& rhs)
{
	DDecQuad result;
	decQuadDivideInteger(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return result;
}

//
//	non member comparison operators
//

//inline bool operator==(double lhs, const DDecQuad& rhs)
//{
//    int exp = decQuadGetExponent(&rhs.decimal_);
//    exp = exp < 0 ? -exp : exp;
//    DDecQuad temp{lhs, exp};
////    std::cout << "exponent: " << decQuadGetExponent(&rhs.decimal_) << '\n';
//	decQuad result;
//	decQuadCompare(&result, &temp.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
//	return decQuadToInt32(&result, &DDecQuad::mCtx_, DEC_ROUND_HALF_EVEN) == 0;
//}
//
//inline bool operator==(const DDecQuad& lhs, double rhs)
//{
//    int exp = decQuadGetExponent(&lhs.decimal_);
//    exp = exp < 0 ? -exp : exp;
//    DDecQuad temp{rhs, exp};
////    std::cout << "exponent: " << decQuadGetExponent(&lhs.decimal_) << '\n';
//	decQuad result;
//	decQuadCompare(&result, &lhs.decimal_, &temp.decimal_, &DDecQuad::mCtx_);
//	return decQuadToInt32(&result, &DDecQuad::mCtx_, DEC_ROUND_HALF_EVEN) == 0;
//}

inline bool operator==(const DDecQuad& lhs, const DDecQuad& rhs)
{
//    if (decQuadGetExponent(&lhs.decimal_) != decQuadGetExponent(&rhs.decimal_))
//    {
//        return false;
//    }
	decQuad result;
	decQuadCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return decQuadToInt32(&result, &DDecQuad::mCtx_, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator!=(const DDecQuad& lhs, const DDecQuad& rhs)
{
	return ! operator==(lhs, rhs);
}

inline bool operator<(const DDecQuad& lhs, const DDecQuad& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return decQuadToInt32(&result, &DDecQuad::mCtx_, DEC_ROUND_HALF_EVEN) == -1;
}

inline bool operator>(const DDecQuad& lhs, const DDecQuad& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return decQuadToInt32(&result, &DDecQuad::mCtx_, DEC_ROUND_HALF_EVEN) == 1;
}

inline bool operator<=(const DDecQuad& lhs, const DDecQuad& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return decQuadToInt32(&result, &DDecQuad::mCtx_, DEC_ROUND_HALF_EVEN) < 1;
}

inline bool operator>=(const DDecQuad& lhs, const DDecQuad& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecQuad::mCtx_);
	return decQuadToInt32(&result, &DDecQuad::mCtx_, DEC_ROUND_HALF_EVEN) >= 0;
}

//
//	stream inserter/extractor
//

inline std::ostream& operator<<(std::ostream& os, const DDecQuad& item)
{
	char output [DECQUAD_String];
	decQuadToString(&item.decimal_, output);
	os << output;
	return os;
}

inline std::istream& operator>>(std::istream& is, DDecQuad& item)
{
	std::string temp;
	is >> temp;
//    std::cout << "temp from stream: " << temp << '\n';
	decQuadFromString(&item.decimal_, temp.c_str(), &DDecQuad::mCtx_);
	return is;
}

}  // namespace DprDecimal

// custom fmtlib formatter for DDecQuad

template <> struct std::formatter<DprDecimal::DDecQuad>: std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const DprDecimal::DDecQuad& number, std::format_context& ctx) const
    {
        std::string s;
		std::format_to(std::back_inserter(s), "{}", number.ToStr());
        return formatter<std::string>::format(s, ctx);
    }
};

#endif

