
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

#include <boost/lexical_cast.hpp>
//#include <boost/algorithm/string/trim.hpp>
//#include <boost/algorithm/string/classification.hpp>

// =====================================================================================
//        Class:  DDecimal
//  Description:  arbitrary number of decimal digits
// =====================================================================================

template < int ddigits >
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


#endif

