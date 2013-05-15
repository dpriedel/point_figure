
// =====================================================================================
//        Class:  DDecimal
//  Description:  Dave's decimal class. wraps libDecnum. Provides only minimal
//				  code to do what I need right now.
// =====================================================================================

#ifndef _DDECIMAL_32_
#define _DDECIMAL_32_

#include "DDecimal.h"

//	example specialization


// =====================================================================================
//        Class:  DDecimal<32>
//  Description:  32 digit decimal class
// =====================================================================================

#include <decQuad.h>

template <>
class DDecimal<32>
{
	friend DDecimal<32> operator+(const DDecimal<32>& lhs, const DDecimal<32>& rhs);
	friend DDecimal<32> operator-(const DDecimal<32>& lhs, const DDecimal<32>& rhs);
	friend DDecimal<32> operator*(const DDecimal<32>& lhs, const DDecimal<32>& rhs);
	friend DDecimal<32> operator/(const DDecimal<32>& lhs, const DDecimal<32>& rhs);

	friend bool operator==(const DDecimal<32>& lhs, const DDecimal<32>& rhs);
	friend bool operator!=(const DDecimal<32>& lhs, const DDecimal<32>& rhs);

	friend bool operator<(const DDecimal<32>& lhs, const DDecimal<32>& rhs);
	friend bool operator>(const DDecimal<32>& lhs, const DDecimal<32>& rhs);

	friend bool operator<=(const DDecimal<32>& lhs, const DDecimal<32>& rhs);
	friend bool operator>=(const DDecimal<32>& lhs, const DDecimal<32>& rhs);

	friend std::istream& operator>>(std::istream& is, DDecimal<32>& item);
	friend std::ostream& operator<<(std::ostream& os, const DDecimal<32>& item);

	public:
		// ====================  LIFECYCLE     =======================================
		DDecimal ();                             // constructor
		DDecimal (const std::string& number);    // constructor
		DDecimal (const char* number);           // constructor
		DDecimal (int32_t number);               // constructor
		DDecimal (uint32_t number);              // constructor

		DDecimal (double number);				 // constructor

		// ====================  ACCESSORS     =======================================
		
		std::string ToStr() const;

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================
		
		DDecimal<32>& operator+=(const DDecimal<32>& rhs);
		DDecimal<32>& operator-=(const DDecimal<32>& rhs);
		DDecimal<32>& operator*=(const DDecimal<32>& rhs);
		DDecimal<32>& operator/=(const DDecimal<32>& rhs);

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================
		
		decQuad mDecimal;
		static decContext mCtx;

}; // -----  end of template class DDecimal16  -----

//
//	constructions/implicit conversion operators
//

	DDecimal<32>::DDecimal()
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECQUAD);
	}

	DDecimal<32>::DDecimal(const char* number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECQUAD);
		decQuadFromString(&this->mDecimal, number, &this->mCtx);
	}

	DDecimal<32>::DDecimal(int32_t number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECQUAD);
		decQuadFromInt32(&this->mDecimal, number);
	}

	DDecimal<32>::DDecimal(uint32_t number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECQUAD);
		decQuadFromInt32(&this->mDecimal, number);
	}

	DDecimal<32>::DDecimal(double number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECQUAD);
		std::string temp = boost::lexical_cast<std::string>(number);
		std::cout << "result of lexical cast: " << temp << std::endl;
		decQuadFromString(&this->mDecimal, temp.c_str(), &this->mCtx);
		decQuadReduce(&this->mDecimal, &this->mDecimal, &this->mCtx);
	}
	
	std::string DDecimal<32>::ToStr() const
	{
		char output [DECQUAD_String];
		decQuadToString(&this->mDecimal, output);
		return std::string(output);
	}

//
//	member arithmetic operators
//

DDecimal<32>& DDecimal<32>::operator+=(const DDecimal<32>& rhs)
{
	decQuadAdd(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

DDecimal<32>& DDecimal<32>::operator-=(const DDecimal<32>& rhs)
{
	decQuadSubtract(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

DDecimal<32>& DDecimal<32>::operator*=(const DDecimal<32>& rhs)
{
	decQuadMultiply(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

DDecimal<32>& DDecimal<32>::operator/=(const DDecimal<32>& rhs)
{
	decQuadDivide(&this->mDecimal, &this->mDecimal, &rhs.mDecimal, &this->mCtx);
	return *this;
}

//
//	non-member arithmetic operators
//

DDecimal<32> operator+(const DDecimal<32>&lhs, const DDecimal<32>& rhs)
{
	DDecimal<32> result;
	decQuadAdd(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &result.mCtx);
	return result;
}

DDecimal<32> operator-(const DDecimal<32>&lhs, const DDecimal<32>& rhs)
{
	DDecimal<32> result;
	decQuadSubtract(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &result.mCtx);
	return result;
}

DDecimal<32> operator*(const DDecimal<32>&lhs, const DDecimal<32>& rhs)
{
	DDecimal<32> result;
	decQuadMultiply(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &result.mCtx);
	return result;
}

DDecimal<32> operator/(const DDecimal<32>&lhs, const DDecimal<32>& rhs)
{
	DDecimal<32> result;
	decQuadDivide(&result.mDecimal, &lhs.mDecimal, &rhs.mDecimal, &result.mCtx);
	return result;
}

//
//	non member comparison operators
//

bool operator==(const DDecimal<32>& lhs, const DDecimal<32>& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &lhs.mCtx);
	if (decQuadIsZero(&result))
		return true;
	else
		return false;
}

bool operator!=(const DDecimal<32>& lhs, const DDecimal<32>& rhs)
{
	return ! operator==(lhs, rhs);
}

bool operator<(const DDecimal<32>& lhs, const DDecimal<32>& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &lhs.mCtx);
	if (decQuadIsNegative(&result))
		return true;
	else
		return false;
}

bool operator>(const DDecimal<32>& lhs, const DDecimal<32>& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &lhs.mCtx);
	if (decQuadIsPositive(&result))
		return true;
	else
		return false;
}

bool operator<=(const DDecimal<32>& lhs, const DDecimal<32>& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &lhs.mCtx);
	if (decQuadIsPositive(&result))
		return false;
	else
		return true;
}

bool operator>=(const DDecimal<32>& lhs, const DDecimal<32>& rhs)
{
	decQuad result;
	decQuadCompare(&result, &lhs.mDecimal, &rhs.mDecimal, &lhs.mCtx);
	if (decQuadIsNegative(&result))
		return false;
	else
		return true;
}

//
//	stream inserter/extractor
//

std::ostream& operator<<(std::ostream& os, const DDecimal<32>& item)
{
	char output [DECQUAD_String];
	decQuadToString(&item.mDecimal, output);
	os << output;
	return os;
}

std::istream& operator>>(std::istream& is, DDecimal<32>& item)
{
	std::string temp;
	is >> temp;
	decQuadFromString(&item.mDecimal, temp.c_str(), &item.mCtx);
	return is;
}


#endif

