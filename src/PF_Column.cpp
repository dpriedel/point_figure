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

#include <fmt/format.h>

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

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Column::PF_Column (const Json::Value& new_data)
{
    this->FromJSON(new_data);
}  // -----  end of method PF_Column::PF_Column  (constructor)  ----- 

std::unique_ptr<PF_Column> PF_Column::MakeReversalColumn (Direction direction, DprDecimal::DDecDouble value,
        tpt the_time)
{
    auto new_column = std::make_unique<PF_Column>(box_size_, reversal_boxes_, fractional_boxes_, direction,
            direction == Direction::e_down ? top_ - box_size_ : value,
            direction == Direction::e_down ? value : bottom_ + box_size_
            );
    new_column->time_span_ = {the_time, the_time};
    return new_column;
}		// -----  end of method PF_Column::MakeReversalColumn  ----- 


PF_Column& PF_Column::operator= (const Json::Value& new_data)
{
    this->FromJSON(new_data);
    return *this;
}		// -----  end of method PF_Column::operator=  ----- 

bool PF_Column::operator== (const PF_Column& rhs) const
{
    return rhs.box_size_ == box_size_ && rhs.reversal_boxes_ == reversal_boxes_ && rhs.direction_ == direction_
        && rhs.top_ == top_ && rhs.bottom_ == bottom_ && rhs.had_reversal_ == had_reversal_;
}		// -----  end of method PF_Column::operator==  ----- 

PF_Column::AddResult PF_Column::AddValue (const DprDecimal::DDecDouble& new_value, tpt the_time)
{
    // if this is the first entry in the column, just set first entry 
    // to the input value rounded down to the nearest box value.

    if (top_ == -1 && bottom_ == -1)
    {
        // OK, first time here for this column.

        top_= RoundDownToNearestBox(new_value);
        bottom_ = top_;
        time_span_ = {the_time, the_time};
    
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
            time_span_.second = the_time;
            return {Status::e_accepted, std::nullopt};
        }
        if (possible_value <= bottom_ - box_size_)
        {
            while(possible_value <= bottom_ - box_size_)
            {
                direction_ = Direction::e_down;
                bottom_ -= box_size_;
            }
            time_span_.second = the_time;
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
            time_span_.second = the_time;
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
                    time_span_.second = the_time;
                    // can't do it as box is occupied.
                    return {Status::e_reversal, MakeReversalColumn(Direction::e_down, top_ - box_size_, the_time)};
                }
                while(possible_value <= bottom_ - box_size_)
                {
                    bottom_ -= box_size_;
                }
                had_reversal_ = true;
                direction_ = Direction::e_down;
                time_span_.second = the_time;
                return {Status::e_accepted, std::nullopt};
            }
            time_span_.second = the_time;
            return {Status::e_reversal, MakeReversalColumn(Direction::e_down, top_ - (box_size_ * reversal_boxes_), the_time)};
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
            time_span_.second = the_time;
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
                    time_span_.second = the_time;
                    // can't do it as box is occupied.
                    return {Status::e_reversal, MakeReversalColumn(Direction::e_up, bottom_ + box_size_, the_time)};
                }
                while(possible_value >= top_ + box_size_)
                {
                    top_ += box_size_;
                }
                had_reversal_ = true;
                direction_ = Direction::e_up;
                time_span_.second = the_time;
                return {Status::e_accepted, std::nullopt};
            }
            time_span_.second = the_time;
            return {Status::e_reversal, MakeReversalColumn(Direction::e_up, bottom_ + (box_size_ * reversal_boxes_), the_time)};
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

    
Json::Value PF_Column::ToJSON () const
{
    Json::Value result;
    result["box_size"] = box_size_.ToStr();
    result["reversal_boxes"] = reversal_boxes_;
    result["bottom"] = bottom_.ToStr();
    result["top"] = top_.ToStr();

    switch(direction_)
    {
        case PF_Column::Direction::e_unknown:
            result["direction"] = "unknown";
            break;

        case PF_Column::Direction::e_down:
            result["direction"] = "down";
            break;

        case PF_Column::Direction::e_up:
            result["direction"] = "up";
            break;
    };

    switch(fractional_boxes_)
    {
        case PF_Column::FractionalBoxes::e_integral:
            result["fractional_boxes"] = "integral";
            break;

        case PF_Column::FractionalBoxes::e_fractional:
            result["fractional_boxes"] = "fractional";
            break;
    };

    result["had_reversal"] = had_reversal_;
//    result["start_at"] = std::to_string(time_span_.first.time_since_epoch().count());
//    result["last_entry"] = std::to_string(time_span_.second.time_since_epoch().count());
    result["start_at"] = time_span_.first.time_since_epoch().count();
    result["last_entry"] = time_span_.second.time_since_epoch().count();

    return result;
}		// -----  end of method PF_Column::ToJSON  ----- 

void PF_Column::FromJSON (const Json::Value& new_data)
{
    box_size_ = DprDecimal::DDecDouble{new_data["box_size"].asString()};
    reversal_boxes_ = new_data["reversal_boxes"].asInt();
    bottom_ = DprDecimal::DDecDouble{new_data["bottom"].asString()};
    top_ = DprDecimal::DDecDouble{new_data["top"].asString()};

    const auto direction = new_data["direction"].asString();
    if (direction == "up")
    {
        direction_ = PF_Column::Direction::e_up;
    }
    else if (direction == "down")
    {
        direction_ = PF_Column::Direction::e_down;
    }
    else if (direction == "unknown")
    {
        direction_ = PF_Column::Direction::e_unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid direction provided: {}. Must be 'up', 'down', 'unknown'.", direction)};
    }

    const auto fractional = new_data["fractional_boxes"].asString();
    if (fractional  == "integral")
    {

        fractional_boxes_ = PF_Column::FractionalBoxes::e_integral;
    }
    else if (direction == "fractional")
    {
        fractional_boxes_ = PF_Column::FractionalBoxes::e_fractional;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid fractional_boxes provided: {}. Must be 'integral' or 'fractional'.", fractional)};
    }

    had_reversal_ = new_data["had_reversal"].asBool();
    
    time_span_.first = PF_Column::tpt{std::chrono::nanoseconds{new_data["start_at"].asInt64()}};
    time_span_.second = PF_Column::tpt{std::chrono::nanoseconds{new_data["last_entry"].asInt64()}};
}		// -----  end of method PF_Column::FromJSON  ----- 

