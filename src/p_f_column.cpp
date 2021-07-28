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

P_F_Column::P_F_Column(int box_size, int reversal_boxes, Direction direction, int32_t top, int32_t bottom)
    : box_size_{box_size}, reversal_boxes_{reversal_boxes}, direction_{direction}, top_{top}, bottom_{bottom}
{
}  // -----  end of method P_F_Column::P_F_Column  (constructor)  -----


P_F_Column::AddResult P_F_Column::AddValue (const DprDecimal::DDecDouble& new_value)
{
    // if this is the first entry in the column, just set first entry 
    // to the input value rounded down to the nearest box value.

    if (top_ == -1 && bottom_ == -1)
    {
        // OK, first time here for this column.

        top_= RoundDownToNearestBox(new_value);
        bottom_ = top_;
    
        return {Status::e_accepted, top_};
    }

    int32_t possible_value = new_value.ToInt();

    // OK, we've got a value but may not yet have a direction.
    // NOTE: Since a new value may gap up or down, we could 
    // have multiple boxes to fill in. Hence the loops in 
    // routines below.

    if (direction_ == Direction::e_unknown)
    {
        // we can compare to either value since they 
        // are both the same at this point.

        if (possible_value >= top_ + box_size_)
        {
            while (possible_value >= top_ + box_size_)
            {
                direction_ = Direction::e_up;
                top_ += box_size_;
            }
            return {Status::e_accepted, top_};
        }
        if (possible_value <= bottom_ - box_size_)
        {
            while(possible_value <= bottom_ - box_size_)
            {
                direction_ = Direction::e_down;
                bottom_ -= box_size_;
            }
            return {Status::e_accepted, bottom_};
        }

        // skip value
        return {Status::e_ignored, possible_value};
    }

    // now that we know our direction, we can either continue in 
    // that direction, ignore the value or reverse our direction 
    // in which case, we start a new column (unless this is 
    // a 1-box reversal and we can revers in place)

    if (direction_ == Direction::e_up)
    {
        if (possible_value >= top_ + box_size_)
        {
            while (possible_value >= top_ + box_size_)
            {
                top_ += box_size_;
            }
            return {Status::e_accepted, top_};
        }
        // look for a reversal 

        if (possible_value <= top_ - (box_size_ * reversal_boxes_))
        {
            // look for one-step-back reversal first.

            if (reversal_boxes_ == 1)
            {
                if (bottom_ <= top_ - box_size_)
                {
                    // can't do it as box is occupied.
                    return {Status::e_reversal, top_ - box_size_};
                }
                while(possible_value <= bottom_ - box_size_)
                {
                    bottom_ -= box_size_;
                }
                had_reversal_ = true;
                direction_ = Direction::e_down;
                return {Status::e_accepted, bottom_};
            }
            return {Status::e_reversal, top_ - (box_size_ * reversal_boxes_)};
        }
    }
    if (direction_ == Direction::e_down)
    {
        if (possible_value <= bottom_ - box_size_)
        {
            while(possible_value <= bottom_ - box_size_)
            {
                bottom_ -= box_size_;
            }
            return {Status::e_accepted, bottom_};
        }
        // look for a reversal 

        if (possible_value >= bottom_ + (box_size_ * reversal_boxes_))
        {
            // look for one-step-back reversal first.

            if (reversal_boxes_ == 1)
            {
                if (top_ >= bottom_ + box_size_)
                {
                    // can't do it as box is occupied.
                    return {Status::e_reversal, bottom_ + box_size_};
                }
                while(possible_value >= top_ + box_size_)
                {
                    top_ += box_size_;
                }
                had_reversal_ = true;
                direction_ = Direction::e_up;
                return {Status::e_accepted, top_};
            }
            return {Status::e_reversal, bottom_ + (box_size_ * reversal_boxes_)};
        }
    }


    return {Status::e_ignored, possible_value};
}		// -----  end of method P_F_Column::AddValue  ----- 

int32_t P_F_Column::RoundDownToNearestBox (const DprDecimal::DDecDouble& a_value) const
{
    int32_t price_as_int = a_value.ToInt();

    // we're using '10' to start with
    int32_t result = (price_as_int / box_size_) * box_size_;

    return result;

}		// -----  end of method P_F_Column::RoundDowntoNearestBox  ----- 

