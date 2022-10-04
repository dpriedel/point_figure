// =====================================================================================
//
//       Filename:  PF_Signals.cpp
//
//    Description:  Place for code related to finding trading 'signals'
//                  in PF_Chart data.
//
//        Version:  1.0
//        Created:  09/21/2022 09:27:22 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#include "PF_Chart.h"
#include "PF_Signals.h"

Json::Value PF_SignalToJSON(const PF_Signal& signal)
{
    return {};
}

PF_Signal PF_SignalFromJSON(const Json::Value& json)
{
    return {};
}

 
bool PF_DoubleTopBuy::operator() (const PF_Chart& chart)
{
	return false;
}		// -----  end of method PF_DoubleTopBuy::operator()  ----- 


