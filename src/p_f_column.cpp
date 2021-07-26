// =====================================================================================
// 
//       Filename:  p_f_column.cpp
// 
//    Description:  implementation of class which implements Point & Figure column data
// 
//        Version:  1.0
//        Created:  2021-07-26 09:36 AM 
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================


#include "p_f_column.h"


//--------------------------------------------------------------------------------------
//       Class:  P_F_Column
//      Method:  P_F_Column
// Description:  constructor
//--------------------------------------------------------------------------------------

P_F_Column::P_F_Column(int box_size, int reversal_size, Direction direction)
    : box_size_{box_size}, reversal_size_{reversal_size}, direction_{direction}
{
}  // -----  end of method P_F_Column::P_F_Column  (constructor)  -----
