// =====================================================================================
// 
//       Filename:  p_f_data.h
// 
//    Description:  class to manage Point & Figure data for a symbol.
// 
//        Version:  1.0
//        Created:  04/30/2013 03:04:50 PM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================

// =====================================================================================
//        Class:  P_F_Data
//  Description:  class to manage Point & Figure data for a symbol.
// =====================================================================================

#include <vector>

#include <boost/date_time/gregorian/gregorian.hpp>

#include "DDecimal_16.h"

namespace greg = boost::gregorian;

class P_F_Column;

class P_F_Data
{
	public:
		// ====================  LIFECYCLE     =======================================
		P_F_Data ();                             // constructor
		P_F_Data(const DDecimal<16> boxsize, int reversal);

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================
		
		void SetBoxSize(const DDecimal<16>& boxsize);
		void SetReversal(int reversal);


		// ====================  OPERATORS     =======================================

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

		std::vector<P_F_Column> mColumns;

		std::string mSymbol;

		greg::date mFirstDate;			//	earliest entry for symbol
		greg::date mLastChangeDate;		//	date of last change to data
		greg::date mLastCheckedDate;	//	last time checked to see if update needed

		DDecimal<16> mBoxSize;
		int mReversalBoxes;

		enum class direction { unknown, moving_up, moving_down };

		direction mCurrentDirection;





}; // -----  end of class P_F_Data  -----

