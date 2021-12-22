// =====================================================================================
//
//       Filename:  Boxes.cpp
//
//    Description:  Manage creation and access to P&F boxes used in charts 
//
//        Version:  1.0
//        Created:  12/22/2021 09:54:51 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#include "Boxes.h"


//--------------------------------------------------------------------------------------
//       Class:  Boxes
//      Method:  Boxes
// Description:  constructor
//--------------------------------------------------------------------------------------
Boxes::Boxes (DprDecimal::DDecQuad box_size, BoxType box_type, BoxScale box_scale)
    : box_size_{box_size}, box_type_{box_type}, box_scale_{box_scale}
{
}  // -----  end of method Boxes::Boxes  (constructor)  ----- 

Boxes::Box Boxes::NewBox (const DprDecimal::DDecQuad& start_at)
{
    return RoundDownToNearestBox(start_at);

}		// -----  end of method Boxes::NewBox  ----- 

Boxes::Box Boxes::RoundDownToNearestBox (const DprDecimal::DDecQuad& a_value) const
{
    DprDecimal::DDecQuad price_as_int;
    if (box_type_ == BoxType::e_integral)
    {
        price_as_int = a_value.ToIntTruncated();
    }
    else
    {
        price_as_int = a_value;
    }

    Box result = DprDecimal::Mod(price_as_int, box_size_) * box_size_;
    return result;

}		// -----  end of method PF_Column::RoundDowntoNearestBox  ----- 

