
// =====================================================================================
//        Class:  DDecimal
//  Description:  Dave's decimal class. wraps libDecnum. Provides only minimal
//				  code to do what I need right now.
// =====================================================================================

#ifndef _DDECIMAL_16_
#define _DDECIMAL_16_

#include <sstream>
#include <iomanip>

#include <decDouble.h>
/* #include <decNumber.h> */
#include "DDecimal.h"
#include "decContext.h"

namespace DprDecimal
{

//	example specialization


// =====================================================================================
//        Class:  DDecimal<16>
//  Description:  16 digit decimal class
// =====================================================================================

constexpr int SMALLDEC = 16;
template <>
class DDecimal<SMALLDEC>
{
	friend DDecimal<SMALLDEC> operator+(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);
	friend DDecimal<SMALLDEC> operator-(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);
	friend DDecimal<SMALLDEC> operator*(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);
	friend DDecimal<SMALLDEC> operator/(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);

	friend bool operator==(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);
	friend bool operator!=(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);

	friend bool operator<(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);
	friend bool operator>(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);

	friend bool operator<=(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);
	friend bool operator>=(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs);

	friend std::istream& operator>>(std::istream& is, DDecimal<SMALLDEC>& item);
	friend std::ostream& operator<<(std::ostream& os, const DDecimal<SMALLDEC>& item);

	public:
		// ====================  LIFECYCLE     =======================================
		DDecimal ();                             // constructor
		DDecimal (const char* number);           // constructor
		DDecimal (const std::string& number);    // constructor
		DDecimal (int32_t number);               // constructor
		DDecimal (uint32_t number);              // constructor

		DDecimal (double number, int dec_digits=2);	 // constructor

		// ====================  ACCESSORS     =======================================
		
		[[nodiscard]] std::string ToStr() const;

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================
		
		DDecimal<SMALLDEC>& operator+=(const DDecimal<SMALLDEC>& rhs);
		DDecimal<SMALLDEC>& operator-=(const DDecimal<SMALLDEC>& rhs);
		DDecimal<SMALLDEC>& operator*=(const DDecimal<SMALLDEC>& rhs);
		DDecimal<SMALLDEC>& operator/=(const DDecimal<SMALLDEC>& rhs);

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

	inline DDecimal<SMALLDEC>::DDecimal()
	{
		decContextDefault(&DDecimal<SMALLDEC>::mCtx, DEC_INIT_DECDOUBLE);
	}

	inline DDecimal<SMALLDEC>::DDecimal(const char* number)
	{
		decContextDefault(&DDecimal<SMALLDEC>::mCtx, DEC_INIT_DECDOUBLE);
		decDoubleFromString(&this->mDecimal, number, &DDecimal<SMALLDEC>::mCtx);
	}

	inline DDecimal<SMALLDEC>::DDecimal(const std::string& number)
		: DDecimal(number.c_str())	{ }

	inline DDecimal<SMALLDEC>::DDecimal(int32_t number)
	{
		decContextDefault(&DDecimal<SMALLDEC>::mCtx, DEC_INIT_DECDOUBLE);
		decDoubleFromInt32(&this->mDecimal, number);
	}

	inline DDecimal<SMALLDEC>::DDecimal(uint32_t number)
	{
		decContextDefault(&DDecimal<SMALLDEC>::mCtx, DEC_INIT_DECDOUBLE);
		decDoubleFromUInt32(&this->mDecimal, number);
	}

	inline DDecimal<SMALLDEC>::DDecimal(double number, int dec_digits)
	{
		decContextDefault(&DDecimal<SMALLDEC>::mCtx, DEC_INIT_DECDOUBLE);
		std::ostringstream temp;
		temp << std::fixed << std::setprecision(dec_digits) << number;

		decDoubleFromString(&this->mDecimal, temp.str().c_str(), &DDecimal<SMALLDEC>::mCtx);
//		decDoubleReduce(&this->mDecimal, &this->mDecimal, &DDecimal<SMALLDEC>::mCtx);
	}
	
	inline std::string DDecimal<SMALLDEC>::ToStr() const
	{
		char output [DECDOUBLE_String];
		decDoubleToString(&this->mDecimal, output);
		return std::string(output);
	}

//
//	member arithmetic operators
//

inline DDecimal<SMALLDEC>& DDecimal<SMALLDEC>::operator+=(const DDecimal<SMALLDEC>& rhs)
{
	decDoubleAdd(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return *this;
}

inline DDecimal<SMALLDEC>& DDecimal<SMALLDEC>::operator-=(const DDecimal<SMALLDEC>& rhs)
{
	decDoubleSubtract(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return *this;
}

inline DDecimal<SMALLDEC>& DDecimal<SMALLDEC>::operator*=(const DDecimal<SMALLDEC>& rhs)
{
	decDoubleMultiply(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return *this;
}

inline DDecimal<SMALLDEC>& DDecimal<SMALLDEC>::operator/=(const DDecimal<SMALLDEC>& rhs)
{
	decDoubleDivide(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return *this;
}

//
//	non-member arithmetic operators
//

inline DDecimal<SMALLDEC> operator+(const DDecimal<SMALLDEC>&lhs, const DDecimal<SMALLDEC>& rhs)
{
	DDecimal<SMALLDEC> result;
	decDoubleAdd(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return result;
}

inline DDecimal<SMALLDEC> operator-(const DDecimal<SMALLDEC>&lhs, const DDecimal<SMALLDEC>& rhs)
{
	DDecimal<SMALLDEC> result;
	decDoubleSubtract(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return result;
}

inline DDecimal<SMALLDEC> operator*(const DDecimal<SMALLDEC>&lhs, const DDecimal<SMALLDEC>& rhs)
{
	DDecimal<SMALLDEC> result;
	decDoubleMultiply(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return result;
}

inline DDecimal<SMALLDEC> operator/(const DDecimal<SMALLDEC>&lhs, const DDecimal<SMALLDEC>& rhs)
{
	DDecimal<SMALLDEC> result;
	decDoubleDivide(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return result;
}

//
//	non member comparison operators
//

inline bool operator==(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return decDoubleToInt32(&result, &DDecimal<SMALLDEC>::mCtx, DEC_ROUND_HALF_EVEN) == 0;
}

inline bool operator!=(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs)
{
	return ! operator==(lhs, rhs);
}

inline bool operator<(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return decDoubleToInt32(&result, &DDecimal<SMALLDEC>::mCtx, DEC_ROUND_HALF_EVEN) == -1;
}

inline bool operator>(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return decDoubleToInt32(&result, &DDecimal<SMALLDEC>::mCtx, DEC_ROUND_HALF_EVEN) == 1;
}

inline bool operator<=(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return decDoubleToInt32(&result, &DDecimal<SMALLDEC>::mCtx, DEC_ROUND_HALF_EVEN) < 1;
}

inline bool operator>=(const DDecimal<SMALLDEC>& lhs, const DDecimal<SMALLDEC>& rhs)
{
	decDouble result;
	decDoubleCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &DDecimal<SMALLDEC>::mCtx);
	return decDoubleToInt32(&result, &DDecimal<SMALLDEC>::mCtx, DEC_ROUND_HALF_EVEN) >= 0;
}

//
//	stream inserter/extractor
//

inline std::ostream& operator<<(std::ostream& os, const DDecimal<SMALLDEC>& item)
{
	char output [DECDOUBLE_String];
	decDoubleToString(&item.mDecimal, output);
	os << output;
	return os;
}

inline std::istream& operator>>(std::istream& is, DDecimal<SMALLDEC>& item)
{
	std::string temp;
	is >> temp;
	decDoubleFromString(&item.mDecimal, temp.c_str(), &DDecimal<SMALLDEC>::mCtx);
	return is;
}

};

#endif

