// =====================================================================================
// 
//       Filename:  p_f_data.cpp
// 
//    Description:  Implementation of class which contains Point & Figure data for a symbol.
// 
//        Version:  2.0
//        Created:  2021-07-29 08:47 AM
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

P_F_Data::P_F_Data (std::string symbol, std::int32_t boxsize, int32_t reversal_boxes)
	: symbol_{symbol}, boxsize_{boxsize}, reversal_boxes_{reversal_boxes}, currentdirection_{Direction::e_unknown}

{
}  // -----  end of method P_F_Data::P_F_Data  (constructor)  -----

void P_F_Data::LoadData (std::istream* input_data)
{
    return ;
}		// -----  end of method P_F_Data::LoadData  ----- 

void P_F_Data::ExportData (std::ostream* output_data)
{
    return ;
}		// -----  end of method P_F_Data::ExportData  ----- 

