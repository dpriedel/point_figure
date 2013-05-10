
// =====================================================================================
//        Class:  DDecimal
//  Description:  Dave's decimal class. wraps libDecnum. Provides only minimal
//				  code to do what I need right now.
// =====================================================================================

#ifndef _DDECIMAL_16_
#define _DDECIMAL_16_

#include "DDecimal.h"

//	example specialization


// =====================================================================================
//        Class:  DDecimal<16>
//  Description:  16 digit decimal class
// =====================================================================================

#include <decDouble.h>

template <>
class DDecimal<16>
{
	friend std::ostream& operator<<(std::ostream& os, const DDecimal<16>& item);
	friend std::istream& operator>>(std::istream& is, DDecimal<16>& item);

	public:
		// ====================  LIFECYCLE     =======================================
		DDecimal ();                             // constructor
		DDecimal (const std::string& number);                             // constructor
		DDecimal (const char* number);                             // constructor
		DDecimal (int32_t number);                             // constructor
		DDecimal (uint32_t number);                             // constructor

		DDecimal (double number);

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================
		
		DDecimal<16>& operator+=(const DDecimal<16>& rhs);
		DDecimal<16>& operator-=(const DDecimal<16>& rhs);
		DDecimal<16>& operator*=(const DDecimal<16>& rhs);
		DDecimal<16>& operator/=(const DDecimal<16>& rhs);

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================
		
		decDouble mDecimal;
		static decContext mCtx;

}; // -----  end of template class DDecimal16  -----

//
//	constructions/implicit conversion operators
//

	DDecimal<16>::DDecimal()
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECDOUBLE);
	}

	DDecimal<16>::DDecimal(const char* number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECDOUBLE);
		decDoubleFromString(&this->mDecimal, number, &this->mCtx);
	}

	DDecimal<16>::DDecimal(int32_t number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECDOUBLE);
		decDoubleFromInt32(&this->mDecimal, number);
	}

	DDecimal<16>::DDecimal(uint32_t number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECDOUBLE);
		decDoubleFromInt32(&this->mDecimal, number);
	}

	DDecimal<16>::DDecimal(double number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECDOUBLE);
		std::string temp = boost::lexical_cast<std::string>(number);
		decDoubleFromString(&this->mDecimal, temp.c_str(), &this->mCtx);
		decDoubleReduce(&this->mDecimal, &this->mDecimal, &this->mCtx);
	}

DDecimal<16>& DDecimal<16>::operator+=(const DDecimal<16>& rhs)
{
	decDoubleAdd(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

DDecimal<16>& DDecimal<16>::operator-=(const DDecimal<16>& rhs)
{
	decDoubleSubtract(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

DDecimal<16>& DDecimal<16>::operator*=(const DDecimal<16>& rhs)
{
	decDoubleMultiply(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

DDecimal<16>& DDecimal<16>::operator/=(const DDecimal<16>& rhs)
{
	decDoubleDivide(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

//
//	arithmetic operators
//

DDecimal<16> operator+(const DDecimal<16>&lhs, const DDecimal<16>& rhs)
{
	DDecimal<16> result = lhs;
	result += rhs;
	return result;
}


//
//	stream inserter/extractor
//

std::ostream& operator<<(std::ostream& os, const DDecimal<16>& item)
{
	char output [DECDOUBLE_String];
	decDoubleToString(&item.mDecimal, output);
	os << output;
	return os;
}

std::istream& operator>>(std::istream& is, DDecimal<16>& item)
{
	std::string temp;
	is >> temp;
	decDoubleFromString(&item.mDecimal, temp.c_str(), &item.mCtx);
	return is;
}


#endif

