
// =====================================================================================
//        Class:  DDecimal
//  Description:  Dave's decimal class. wraps libDecnum. Provides only minimal
//				  code to do what I need right now.
// =====================================================================================

#ifndef _DDECIMAL_
#define _DDECIMAL_

#include <iosfwd>
#include <string>
#include <decNumber.h>

template < int bits >
class DDecimal
{
	public:
		// ====================  LIFECYCLE     =======================================
		DDecimal ();                             // constructor
		DDecimal (const std::string& number);                             // constructor
		DDecimal (const char* number);                             // constructor

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================
		
	decNumber mDecimal;
	static decContext mCtx;

}; // -----  end of template class DDecimal  -----

//	example specialization


// =====================================================================================
//        Class:  DDecimal
//  Description:  64 bit decimal class
// =====================================================================================

#include <decDouble.h>

template <>
class DDecimal<64>
{
	friend std::ostream& operator<<(std::ostream& os, const DDecimal<64>& item);
	friend std::istream& operator>>(std::istream& is, DDecimal<64>& item);

	public:
		// ====================  LIFECYCLE     =======================================
		DDecimal ();                             // constructor
		DDecimal (const std::string& number);                             // constructor
		DDecimal (const char* number);                             // constructor

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================
		
		decDouble mDecimal;
		static decContext mCtx;

}; // -----  end of template class DDecimal64  -----


	DDecimal<64>::DDecimal()
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECDOUBLE);
	}

	DDecimal<64>::DDecimal(const char* number)
	{
		decContextDefault(&this->mCtx, DEC_INIT_DECDOUBLE);
		decDoubleFromString(&this->mDecimal, number, &this->mCtx);
	}

std::ostream& operator<<(std::ostream& os, const DDecimal<64>& item)
{
	char output [DECDOUBLE_String];
	decDoubleToString(&item.mDecimal, output);
	os << output;
	return os;
}
std::istream& operator>>(std::istream& is, DDecimal<64>& item)
{
	std::string temp;
	is >> temp;
	decDoubleFromString(&item.mDecimal, temp.c_str(), &item.mCtx);
	return is;
}


#endif

