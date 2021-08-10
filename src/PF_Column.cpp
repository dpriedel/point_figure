// =====================================================================================
// 
//       Filename:  PF_Column.cpp
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

#include <exception>
#include <memory>

#include "PF_Column.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Column::PF_Column(int box_size, int reversal_boxes, Direction direction, int32_t top, int32_t bottom)
    : box_size_{box_size}, reversal_boxes_{reversal_boxes}, direction_{direction}, top_{top}, bottom_{bottom}
{
}  // -----  end of method PF_Column::PF_Column  (constructor)  -----

std::unique_ptr<PF_Column> PF_Column::MakeReversalColumn (Direction direction, int32_t value )
{
    return std::make_unique<PF_Column>(box_size_, reversal_boxes_, direction,
            direction == Direction::e_down ? top_ - box_size_ : value,
            direction == Direction::e_down ? value : bottom_ + box_size_
            );
}		// -----  end of method PF_Column::MakeReversalColumn  ----- 

PF_Column::AddResult PF_Column::AddValue (const DprDecimal::DDecDouble& new_value)
{
    // if this is the first entry in the column, just set first entry 
    // to the input value rounded down to the nearest box value.

    if (top_ == -1 && bottom_ == -1)
    {
        // OK, first time here for this column.

        top_= RoundDownToNearestBox(new_value);
        bottom_ = top_;
    
        return {Status::e_accepted, std::nullopt};
    }

    // du Plessis says in "Definitive Guide" (pp. 28-29) that fractional 
    // parts of price were just truncated, no rounding done.  I'm going to go 
    // with rounding here though since, at the time he was talking about,
    // there were no computers to do this.

    // Nevermind about above commment for now

    int32_t possible_value = new_value.ToIntTruncated();

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
            return {Status::e_accepted, std::nullopt};
        }
        if (possible_value <= bottom_ - box_size_)
        {
            while(possible_value <= bottom_ - box_size_)
            {
                direction_ = Direction::e_down;
                bottom_ -= box_size_;
            }
            return {Status::e_accepted, std::nullopt};
        }

        // skip value
        return {Status::e_ignored, std::nullopt};
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
            return {Status::e_accepted, std::nullopt};
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
                    return {Status::e_reversal, MakeReversalColumn(Direction::e_down, top_ - box_size_)};
                }
                while(possible_value <= bottom_ - box_size_)
                {
                    bottom_ -= box_size_;
                }
                had_reversal_ = true;
                direction_ = Direction::e_down;
                return {Status::e_accepted, std::nullopt};
            }
            return {Status::e_reversal, MakeReversalColumn(Direction::e_down, top_ - (box_size_ * reversal_boxes_))};
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
            return {Status::e_accepted, std::nullopt};
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
                    return {Status::e_reversal, MakeReversalColumn(Direction::e_up, bottom_ + box_size_)};
                }
                while(possible_value >= top_ + box_size_)
                {
                    top_ += box_size_;
                }
                had_reversal_ = true;
                direction_ = Direction::e_up;
                return {Status::e_accepted, std::nullopt};
            }
            return {Status::e_reversal, MakeReversalColumn(Direction::e_up, bottom_ + (box_size_ * reversal_boxes_))};
        }
    }


    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::AddValue  ----- 

int32_t PF_Column::RoundDownToNearestBox (const DprDecimal::DDecDouble& a_value) const
{
    int32_t price_as_int = a_value.ToIntTruncated();

    // we're using '10' to start with
    int32_t result = (price_as_int / box_size_) * box_size_;

    return result;

}		// -----  end of method PF_Column::RoundDowntoNearestBox  ----- 

