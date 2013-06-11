// =====================================================================================
// 
//       Filename:  p_f_data.cpp
// 
//    Description:  Implementation of class which contains Point & Figure data for a symbol.
// 
//        Version:  1.0
//        Created:  05/21/2013 01:58:39 PM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================


#include "p_f_data.h"
#include "p_f_column.h"

//--------------------------------------------------------------------------------------
//       Class:  P_F_Data
//      Method:  P_F_Data
// Description:  constructor
//--------------------------------------------------------------------------------------

P_F_Data::P_F_Data ()
	: mBoxSize{0}, mReversalBoxes{0}, mCurrentDirection{direction::unknown}

{
}  // -----  end of method P_F_Data::P_F_Data  (constructor)  -----

