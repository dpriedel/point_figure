
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
		
		decDouble mDecimal{};
		static decContext mCtx;

}; // -----  end of template class DDecimalSMALLDEC  -----

//
//	constructions/implicit conversion operators
//
	
inline std::string DDecDouble::ToStr() const
{
    char output [DECDOUBLE_String];
    decDoubleToString(&this->mDecimal, output);
    return std::string(output);
}

//
//	member arithmetic operators
//

inline DDecDouble& DDecDouble::operator+=(const DDecDouble& rhs)
{
	decDoubleAdd(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return *this;
}

inline DDecDouble& DDecDouble::operator-=(const DDecDouble& rhs)
{
	decDoubleSubtract(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return *this;
}

inline DDecDouble& DDecDouble::operator*=(const DDecDouble& rhs)
{
	decDoubleMultiply(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return *this;
}

inline DDecDouble& DDecDouble::operator/=(const DDecDouble& rhs)
{
	decDoubleDivide(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return *this;
}

//
//	non-member arithmetic operators
//

inline DDecDouble operator+(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleAdd(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return result;
}

inline DDecDouble operator-(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleSubtract(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return result;
}

inline DDecDouble operator*(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleMultiply(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return result;
}

inline DDecDouble operator/(const DDecDouble&lhs, const DDecDouble& rhs)
{
	DDecDouble result;
	decDoubleDivide(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return result;
}

//
//	non member comparison operators
//

inline bool operator==(double lhs, const DDecDouble& rhs)
{
    int exp = decDoubleGetExponent(&rhs.mDecimal);
    exp = exp < 0 ? -exp : exp;
    DDecDouble temp{lhs, exp};
//    std::cout << "exponent: " << decDoubleGetExponent(&rhs.mDecimal) << '\n';
	decDouble result;
	decDoubleCompare(&result, &temp.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator==(const DDecDouble& lhs, double rhs)
{
    int exp = decDoubleGetExponent(&lhs.mDecimal);
    exp = exp < 0 ? -exp : exp;
    DDecDouble temp{rhs, exp};
//    std::cout << "exponent: " << decDoubleGetExponent(&lhs.mDecimal) << '\n';
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &temp.mDecimal, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator==(const DDecDouble& lhs, const DDecDouble& rhs)
{
//    if (decDoubleGetExponent(&lhs.mDecimal) != decDoubleGetExponent(&rhs.mDecimal))
//    {
//        return false;
//    }
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator!=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	return ! operator==(lhs, rhs);
}

inline bool operator<(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == -1;
}

inline bool operator>(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) == 1;
}

inline bool operator<=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) < 1;
}

inline bool operator>=(const DDecDouble& lhs, const DDecDouble& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecDouble::mCtx);
	return decDoubleToInt32(&result, &DDecDouble::mCtx, DEC_ROUND_HALF_EVEN) >= 0;
}

//
//	stream inserter/extractor
//

inline std::ostream& operator<<(std::ostream& os, const DDecDouble& item)
{
	char output [DECDOUBLE_String];
	decDoubleToString(&item.mDecimal, output);
	os << output;
	return os;
}

inline std::istream& operator>>(std::istream& is, DDecDouble& item)
{
	std::string temp;
	is >> temp;
	decDoubleFromString(&item.mDecimal, temp.c_str(), &DDecDouble::mCtx);
	return is;
}

};

#endif

