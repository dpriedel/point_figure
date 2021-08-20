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

#include "DDecDouble.h"
#include "PF_Column.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Column::PF_Column(DprDecimal::DDecDouble box_size, int reversal_boxes, FractionalBoxes fractional_boxes,
            Direction direction, DprDecimal::DDecDouble top, DprDecimal::DDecDouble bottom)
    : box_size_{box_size}, reversal_boxes_{reversal_boxes}, fractional_boxes_{fractional_boxes},
        direction_{direction}, top_{top}, bottom_{bottom}
{
}  // -----  end of method PF_Column::PF_Column  (constructor)  -----

std::unique_ptr<PF_Column> PF_Column::MakeReversalColumn (Direction direction, DprDecimal::DDecDouble value )
{
    return std::make_unique<PF_Column>(box_size_, reversal_boxes_, fractional_boxes_, direction,
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

    // In "21st Century Point and Figure", Du Plessis talks about
    // use the Average True Range to set the box size. This seems
    // to me to be particularly appropriate for live streaming data.
    // It also will support logarithmic charts.

    DprDecimal::DDecDouble possible_value;
    if (fractional_boxes_ == FractionalBoxes::e_integral)
    {
        possible_value = new_value.ToIntTruncated();
    }
    else
    {
        possible_value = new_value;
    }

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

DprDecimal::DDecDouble PF_Column::RoundDownToNearestBox (const DprDecimal::DDecDouble& a_value) const
{
    DprDecimal::DDecDouble price_as_int;
    if (fractional_boxes_ == FractionalBoxes::e_integral)
    {
        price_as_int = a_value.ToIntTruncated();
    }
    else
    {
        price_as_int = a_value;
    }

    // we're using '10' to start with
    DprDecimal::DDecDouble result = DprDecimal::Mod(price_as_int, box_size_) * box_size_;

    return result;

}		// -----  end of method PF_Column::RoundDowntoNearestBox  ----- 

