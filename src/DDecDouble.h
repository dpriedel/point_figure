
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
    DDecDouble (const std::string& number);    // constructor
    DDecDouble (int32_t number);               // constructor
    DDecDouble (uint32_t number);              // constructor

    DDecDouble (double number, int dec_digits=2);	 // constructor

    // ====================  ACCESSORS     =======================================
    
    [[nodiscard]] std::string ToStr() const;
    int32_t ToIntRounded() const;
    int32_t ToIntTruncated() const;
    double ToDouble() const;

    // ====================  MUTATORS      =======================================

    // ====================  OPERATORS     =======================================
    
    DDecDouble& operator+=(const DDecDouble& rhs);
    DDecDouble& operator-=(const DDecDouble& rhs);
    DDecDouble& operator*=(const DDecDouble& rhs);
    DDecDouble& operator/=(const DDecDouble& rhs);

protected:
    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  DATA MEMBERS  =======================================
    
    decDouble decimal_{};
    static decContext mCtx;

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
	decDoubleAdd(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return *this;
}

inline DDecDouble& DDecDouble::operator-=(const DDecDouble& rhs)
{
	decDoubleSubtract(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return *this;
}

inline DDecDouble& DDecDouble::operator*=(const DDecDouble& rhs)
{
	decDoubleMultiply(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return *this;
}

inline DDecDouble& DDecDouble::operator/=(const DDecDouble& rhs)
{
	decDoubleDivide(&this->decimal_, &this->decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return *this;
}

//
//	non-member arithmetic operators
//

inline DDecDouble operator+(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleAdd(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return result;
}

inline DDecDouble operator-(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleSubtract(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return result;
}

inline DDecDouble operator*(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleMultiply(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return result;
}

inline DDecDouble operator/(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleDivide(&result.decimal_, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
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
	decDoubleCompare(&result, &temp.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator==(const DDecDouble& lhs, double rhs)
{
    int exp = decDoubleGetExponent(&lhs.decimal_);
    exp = exp < 0 ? -exp : exp;
    DDecDouble temp{rhs, exp};
//    std::cout << "exponent: " << decDoubleGetExponent(&lhs.decimal_) << '\n';
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &temp.decimal_, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator==(const DDecDouble& lhs, const DDecDouble& rhs)
{
//    if (decDoubleGetExponent(&lhs.decimal_) != decDoubleGetExponent(&rhs.decimal_))
//    {
//        return false;
//    }
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator!=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	return ! operator==(lhs, rhs);
}

inline bool operator<(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == -1;
}

inline bool operator>(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 1;
}

inline bool operator<=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) < 1;
}

inline bool operator>=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.decimal_, &rhs.decimal_, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) >= 0;
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
	decDoubleFromString(&item.decimal_, temp.c_str(), &DDecDouble::mCtx);
	return is;
}

};

#endif

