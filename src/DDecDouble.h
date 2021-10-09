
// =====================================================================================
//        Class:  DDecDouble
//  Description:  Dave's decimal class. wraps libDecnum. Provides only minimal
//				  code to do what I need right now.
// =====================================================================================

#ifndef _DDECDOUBLE_
#define _DDECDOUBLE_

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string_view>

extern "C" {
    #include <decDouble.h>
}

namespace DprDecimal
{

//	example specialization


// =====================================================================================
//        Class:  DDecDouble
//  Description:  64 digit decimal class
// =====================================================================================

class DDecDouble
{
	friend DDecDouble operator+(const DDecDouble& lhs, const DDecDouble& rhs);
	friend DDecDouble operator-(const DDecDouble& lhs, const DDecDouble& rhs);
	friend DDecDouble operator*(const DDecDouble& lhs, const DDecDouble& rhs);
	friend DDecDouble operator/(const DDecDouble& lhs, const DDecDouble& rhs);

    friend DDecDouble Mod(const DDecDouble&lhs, const DDecDouble& rhs);

	friend bool operator==(const DDecDouble& lhs, double rhs);
	friend bool operator==(double lhs, const DDecDouble& rhs);
	friend bool operator==(const DDecDouble& lhs, const DDecDouble& rhs);
	friend bool operator!=(const DDecDouble& lhs, const DDecDouble& rhs);

	friend bool operator<(const DDecDouble& lhs, const DDecDouble& rhs);
	friend bool operator>(const DDecDouble& lhs, const DDecDouble& rhs);

	friend bool operator<=(const DDecDouble& lhs, const DDecDouble& rhs);
	friend bool operator>=(const DDecDouble& lhs, const DDecDouble& rhs);

	friend std::istream& operator>>(std::istream& is, DDecDouble& item);
	friend std::ostream& operator<<(std::ostream& os, const DDecDouble& item);

public:
    // ====================  LIFECYCLE     =======================================
    DDecDouble ();                             // constructor
    DDecDouble (const char* number);           // constructor
    DDecDouble (std::string_view number);    // constructor
    DDecDouble (int32_t number);               // constructor
    DDecDouble (uint32_t number);              // constructor

    DDecDouble (double number, int dec_digits=2);	 // constructor

    // ====================  ACCESSORS     =======================================
    
    [[nodiscard]] std::string ToStr() const;
    [[nodiscard]] int32_t ToIntRounded() const;
    [[nodiscard]] int32_t ToIntTruncated() const;
    [[nodiscard]] double ToDouble() const;

    // ====================  MUTATORS      =======================================

    // ====================  OPERATORS     =======================================
    
    DDecDouble& operator+=(const DDecDouble& rhs);
    DDecDouble& operator-=(const DDecDouble& rhs);
    DDecDouble& operator*=(const DDecDouble& rhs);
    DDecDouble& operator/=(const DDecDouble& rhs);

    DDecDouble& operator=(int32_t rhs);
    DDecDouble& operator=(uint32_t rhs);
    DDecDouble& operator=(double rhs);
    DDecDouble& operator=(std::string_view rhs);

protected:
    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  DATA MEMBERS  =======================================
    
    decDouble decimal_{};
    static decContext mCtx_;

}; // -----  end of template class DDecimalSMALLDEC  -----

//
//	constructions/implicit conversion operators
//
	
inline std::string DDecDouble::ToStr() const
{
    char output [DECDOUBLE_String];
    decDoubleToString(&this->decimal_, output);
    return std::string(output);
}

//
//	member arithmetic operators
//

inline DDecDouble& DDecDouble::operator+=(const DDecDouble& rhs)
{
	decDoubleAdd(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return *this;
}

inline DDecDouble& DDecDouble::operator-=(const DDecDouble& rhs)
{
	decDoubleSubtract(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return *this;
}

inline DDecDouble& DDecDouble::operator*=(const DDecDouble& rhs)
{
	decDoubleMultiply(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return *this;
}

inline DDecDouble& DDecDouble::operator/=(const DDecDouble& rhs)
{
	decDoubleDivide(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return *this;
}

//
//	non-member arithmetic operators
//

inline DDecDouble operator+(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleAdd(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return result;
}

inline DDecDouble operator-(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleSubtract(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return result;
}

inline DDecDouble operator*(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleMultiply(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return result;
}

inline DDecDouble operator/(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleDivide(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return result;
}

inline DDecDouble Mod(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleDivideInteger(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return result;
}

//
//	non member comparison operators
//

inline bool operator==(double lhs, const DDecDouble& rhs)
{
    int exp = decDoubleGetExponent(&rhs.decimal_);
    exp = exp < 0 ? -exp : exp;
    DDecDouble temp{lhs, exp};
//    std::cout << "exponent: " << decDoubleGetExponent(&rhs.decimal_) << '\n';
	decDouble result;
	decDoubleCompare(&result, &temp.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return decDoubleToInt32(&result, &DDecDouble::mCtx_, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator==(const DDecDouble& lhs, double rhs)
{
    int exp = decDoubleGetExponent(&lhs.decimal_);
    exp = exp < 0 ? -exp : exp;
    DDecDouble temp{rhs, exp};
//    std::cout << "exponent: " << decDoubleGetExponent(&lhs.decimal_) << '\n';
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &temp.decimal_, &DDecDouble::mCtx_);
	return decDoubleToInt32(&result, &DDecDouble::mCtx_, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator==(const DDecDouble& lhs, const DDecDouble& rhs)
{
//    if (decDoubleGetExponent(&lhs.decimal_) != decDoubleGetExponent(&rhs.decimal_))
//    {
//        return false;
//    }
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return decDoubleToInt32(&result, &DDecDouble::mCtx_, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator!=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	return ! operator==(lhs, rhs);
}

inline bool operator<(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return decDoubleToInt32(&result, &DDecDouble::mCtx_, DEC_ROUND_HALF_EVEN) == -1;
}

inline bool operator>(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return decDoubleToInt32(&result, &DDecDouble::mCtx_, DEC_ROUND_HALF_EVEN) == 1;
}

inline bool operator<=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return decDoubleToInt32(&result, &DDecDouble::mCtx_, DEC_ROUND_HALF_EVEN) < 1;
}

inline bool operator>=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx_);
	return decDoubleToInt32(&result, &DDecDouble::mCtx_, DEC_ROUND_HALF_EVEN) >= 0;
}

//
//	stream inserter/extractor
//

inline std::ostream& operator<<(std::ostream& os, const DDecDouble& item)
{
	char output [DECDOUBLE_String];
	decDoubleToString(&item.decimal_, output);
	os << output;
	return os;
}

inline std::istream& operator>>(std::istream& is, DDecDouble& item)
{
	std::string temp;
	is >> temp;
//    std::cout << "temp from stream: " << temp << '\n';
	decDoubleFromString(&item.decimal_, temp.c_str(), &DDecDouble::mCtx_);
	return is;
}

};

#endif

