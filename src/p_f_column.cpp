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

//-----------------------------------------------------------------------------
// 
// Logic for how to construct a column given incoming data comes from the
// excellent book "The Definitive Guide to Point and Figure" by
// Jeremy du Plessis.
// This book is written by an engineer/programmer to the alogorithm descriptions
// can be readily used to write code.  However, the implementation along with 
// responsibility for any errors is mine.
//
//-----------------------------------------------------------------------------

#include <cstdint>
#include <exception>

#include "p_f_column.h"

//--------------------------------------------------------------------------------------
//       Class:  P_F_Column
//      Method:  P_F_Column
// Description:  constructor
//--------------------------------------------------------------------------------------

P_F_Column::P_F_Column(int box_size, int reversal_boxes, Direction direction)
    : box_size_{box_size}, reversal_boxes_{reversal_boxes}, direction_{direction}
{
}  // -----  end of method P_F_Column::P_F_Column  (constructor)  -----


bool P_F_Column::AddValue (const DprDecimal::DDecDouble& new_value)
{
    // if this is the first entry in the column, just set first entry 
    // to the input value rounded down to the nearest box value.

    std::cout << "new value: " << new_value << '\n';

    if (top_ == -1 && bottom_ == -1)
    {
        // OK, first time here for this column.

        top_= RoundDownToNearestBox(new_value);
        bottom_ = top_;
    
        return true;
    }

    int32_t possible_value = new_value.ToInt();

    std::cout << "possible value: " << possible_value << '\n';

    // OK, we've got a value but may not yet have a direction.

    if (direction_ == Direction::e_unknown)
    {
        // we can compare to either value since they 
        // are both the same at this point.

        if (possible_value >= top_ + box_size_)
        {
            while (possible_value > top_)
            {
                direction_ = Direction::e_up;
                top_ += box_size_;
            }
            return true;
        }
        if (possible_value <= bottom_ - box_size_)
        {
            while(possible_value < bottom_)
            {
                direction_ = Direction::e_down;
                bottom_ -= box_size_;
            }
            return true;
        }
    }

    // now that we know our direction, we can either continue in 
    // that direction, ignore the value or reverse our direction 
    // in which case, we start a new column (unless this is 
    // a 1-box reversal and we can revers in place)

    if (direction_ == Direction::e_up)
    {
        if (possible_value >= top_ + box_size_)
        {
            while (possible_value > top_)
            {
                top_ += box_size_;
            }
            return true;
        }
        // look for a reversal 

        if (possible_value <= top_ - (box_size_ * reversal_boxes_))
        {
            throw std::runtime_error("got reversal from going up.\n");
        }
    }
    if (direction_ == Direction::e_down)
    {
        if (possible_value <= bottom_ - box_size_)
        {
            while(possible_value < bottom_)
            {
                bottom_ -= box_size_;
            }
            return true;
        }
        // look for a reversal 

        if (possible_value >= bottom_ + (box_size_ * reversal_boxes_))
        {
            throw std::runtime_error("got reversal from going down.\n");
        }
    }


    return false;
}		// -----  end of method P_F_Column::AddValue  ----- 

int32_t P_F_Column::RoundDownToNearestBox (const DprDecimal::DDecDouble& a_value) const
{
    int32_t price_as_int = a_value.ToInt();

    // we're using '10' to start with
    int32_t result = (price_as_int / box_size_) * box_size_;

    return result;

}		// -----  end of method P_F_Column::RoundDowntoNearestBox  ----- 

