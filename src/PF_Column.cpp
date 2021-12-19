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

#include "DDecQuad.h"
#include "PF_Column.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Column::PF_Column(const DprDecimal::DDecQuad& box_size, int reversal_boxes, FractionalBoxes fractional_boxes,
            ColumnScale column_scale, Direction direction, DprDecimal::DDecQuad top, DprDecimal::DDecQuad bottom,
            BoxSizePerCent boxsize_percent)
    : box_size_{box_size}, reversal_boxes_{reversal_boxes}, fractional_boxes_{fractional_boxes},
        column_scale_{column_scale},
        direction_{direction}, top_{top}, bottom_{bottom},
        boxsize_percent_{boxsize_percent}
{
    if (column_scale_ == ColumnScale::e_logarithmic)
    {
        log_box_increment_ = (1.0 + box_size_).log_n();
    }

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

PF_Column PF_Column::MakeReversalColumn (Direction direction, DprDecimal::DDecQuad value,
        tpt the_time)
{
    auto new_column = PF_Column{box_size_, reversal_boxes_, fractional_boxes_, column_scale_, direction,
            direction == Direction::e_down ? top_ - box_size_ : value,
            direction == Direction::e_down ? value : bottom_ + box_size_,
            boxsize_percent_
    };
    new_column.time_span_ = {the_time, the_time};
    return new_column;
}		// -----  end of method PF_Column::MakeReversalColumn  ----- 

PF_Column PF_Column::MakeReversalColumnLog (Direction direction, DprDecimal::DDecQuad value,
        tpt the_time)
{
    auto new_column = PF_Column{box_size_, reversal_boxes_, fractional_boxes_, column_scale_, direction,
            direction == Direction::e_down ? top_ - log_box_increment_ : value,
            direction == Direction::e_down ? value : bottom_ + log_box_increment_
    };
    new_column.time_span_ = {the_time, the_time};
    return new_column;
}		// -----  end of method PF_Column::MakeReversalColumnLog  ----- 


PF_Column& PF_Column::operator= (const Json::Value& new_data)
{
    this->FromJSON(new_data);
    return *this;
}		// -----  end of method PF_Column::operator=  ----- 

bool PF_Column::operator== (const PF_Column& rhs) const
{
    return rhs.box_size_ == box_size_ && rhs.reversal_boxes_ == reversal_boxes_ && rhs.direction_ == direction_
        && rhs.column_scale_ == column_scale_ 
        && rhs.top_ == top_ && rhs.bottom_ == bottom_ && rhs.had_reversal_ == had_reversal_;
}		// -----  end of method PF_Column::operator==  ----- 

PF_Column::AddResult PF_Column::AddValue (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    if (column_scale_ == ColumnScale::e_logarithmic)
    {
        return AddValueLog(new_value.log_n(), the_time);
    }

    if (top_ == -1 && bottom_ == -1)
    {
        // OK, first time here for this column.

        return StartColumn(new_value, the_time);
    }

    DprDecimal::DDecQuad possible_value;
    if (fractional_boxes_ == FractionalBoxes::e_integral)
    {
        possible_value = new_value.ToIntTruncated();
    }
    else
    {
        possible_value = new_value;
    }

    // OK, we've got a value but may not yet have a direction.

    if (direction_ == Direction::e_unknown)
    {
        return TryToFindDirection(possible_value, the_time);
    }

    // If we're here, we have direction. We can either continue in 
    // that direction, ignore the value or reverse our direction 
    // in which case, we start a new column (unless this is 
    // a 1-box reversal and we can revers in place)

    if (direction_ == Direction::e_up)
    {
        return TryToExtendUp(possible_value, the_time);
    }
    return TryToExtendDown(possible_value, the_time);
}		// -----  end of method PF_Column::AddValue  ----- 

PF_Column::AddResult PF_Column::StartColumn (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    // As this is the first entry in the column, just set fields 
    // to the input value rounded down to the nearest box value.

    top_= RoundDownToNearestBox(new_value);
    bottom_ = top_;
    time_span_ = {the_time, the_time};

    return {Status::e_accepted, std::nullopt};
}		// -----  end of method PF_Column::StartColumn  ----- 


PF_Column::AddResult PF_Column::TryToFindDirection (const DprDecimal::DDecQuad& possible_value, tpt the_time)
{
    // NOTE: Since a new value may gap up or down, we could 
    // have multiple boxes to fill in. 

    // we can compare to either value since they 
    // are both the same at this point.

    if (possible_value >= top_ + box_size_)
    {
        direction_ = Direction::e_up;
        int how_many_boxes = ((possible_value - top_) / box_size_).ToIntTruncated();
        top_ += how_many_boxes * box_size_;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }
    if (possible_value <= bottom_ - box_size_)
    {
        direction_ = Direction::e_down;
        int how_many_boxes = ((possible_value - bottom_) / box_size_).ToIntTruncated();
        bottom_ += how_many_boxes * box_size_;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }

    // skip value
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToFindDirection  ----- 

PF_Column::AddResult PF_Column::TryToExtendUp (const DprDecimal::DDecQuad& possible_value, tpt the_time)
{
    if (possible_value >= top_ + box_size_)
    {
        int how_many_boxes = ((possible_value - top_) / box_size_).ToIntTruncated();
        top_ += how_many_boxes * box_size_;
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
            int how_many_boxes = ((possible_value - bottom_) / box_size_).ToIntTruncated();
            bottom_ += how_many_boxes * box_size_;
            had_reversal_ = true;
            direction_ = Direction::e_down;
            time_span_.second = the_time;
            return {Status::e_accepted, std::nullopt};
        }
        time_span_.second = the_time;
        return {Status::e_reversal, MakeReversalColumn(Direction::e_down, top_ - (box_size_ * reversal_boxes_), the_time)};
    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Chart::TryToExtendUp  ----- 


PF_Column::AddResult PF_Column::TryToExtendDown (const DprDecimal::DDecQuad& possible_value, tpt the_time)
{
    if (possible_value <= bottom_ - box_size_)
    {
        int how_many_boxes = ((possible_value - bottom_) / box_size_).ToIntTruncated();
        bottom_ += how_many_boxes * box_size_;
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
            int how_many_boxes = ((possible_value - top_) / box_size_).ToIntTruncated();
            top_ += how_many_boxes * box_size_;
            had_reversal_ = true;
            direction_ = Direction::e_up;
            time_span_.second = the_time;
            return {Status::e_accepted, std::nullopt};
        }
        time_span_.second = the_time;
        return {Status::e_reversal, MakeReversalColumn(Direction::e_up, bottom_ + (box_size_ * reversal_boxes_), the_time)};
    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToExtendDown  ----- 

PF_Column::AddResult PF_Column::AddValueLog (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    if (top_ == -1 && bottom_ == -1)
    {
        // OK, first time here for this column.

        return StartColumnLog(new_value, the_time);
    }

    // OK, we've got a value but may not yet have a direction.

    if (direction_ == Direction::e_unknown)
    {
        return TryToFindDirectionLog(new_value, the_time);
    }

    // If we're here, we have direction. We can either continue in 
    // that direction, ignore the value or reverse our direction 
    // in which case, we start a new column (unless this is 
    // a 1-box reversal and we can revers in place)

    if (direction_ == Direction::e_up)
    {
        return TryToExtendUpLog(new_value, the_time);
    }
    return TryToExtendDownLog(new_value, the_time);
}		// -----  end of method PF_Column::AddValue  ----- 

PF_Column::AddResult PF_Column::StartColumnLog (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    // As this is the first entry in the column, just set fields 
    // to the input value rounded down to the nearest box value.

    top_ = new_value;
    bottom_ = top_;
    time_span_ = {the_time, the_time};

    return {Status::e_accepted, std::nullopt};
}		// -----  end of method PF_Column::StartColumn  ----- 


PF_Column::AddResult PF_Column::TryToFindDirectionLog (const DprDecimal::DDecQuad& possible_value, tpt the_time)
{
    // NOTE: Since a new value may gap up or down, we could 
    // have multiple boxes to fill in. 

    // we can compare to either value since they 
    // are both the same at this point.

    if (possible_value >= top_ + log_box_increment_)
    {
        direction_ = Direction::e_up;
        int how_many_boxes = ((possible_value - top_) / log_box_increment_).ToIntTruncated();
        top_ += how_many_boxes * log_box_increment_;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }
    if (possible_value <= bottom_ - log_box_increment_)
    {
        direction_ = Direction::e_down;
        int how_many_boxes = ((possible_value - bottom_) / log_box_increment_).ToIntTruncated();
        bottom_ += how_many_boxes * log_box_increment_;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }

    // skip value
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToFindDirection  ----- 

PF_Column::AddResult PF_Column::TryToExtendUpLog (const DprDecimal::DDecQuad& possible_value, tpt the_time)
{
    if (possible_value >= top_ + log_box_increment_)
    {
        int how_many_boxes = ((possible_value - top_) / log_box_increment_).ToIntTruncated();
        top_ += how_many_boxes * log_box_increment_;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }
    // look for a reversal 

    if (possible_value <= top_ - (log_box_increment_ * reversal_boxes_))
    {
        // look for one-step-back reversal first.

        if (reversal_boxes_ == 1)
        {
            if (bottom_ <= top_ - log_box_increment_)
            {
                time_span_.second = the_time;
                // can't do it as box is occupied.
                return {Status::e_reversal, MakeReversalColumnLog(Direction::e_down, top_ - log_box_increment_, the_time)};
            }
            int how_many_boxes = ((possible_value - bottom_) / log_box_increment_).ToIntTruncated();
            bottom_ += how_many_boxes * log_box_increment_;
            had_reversal_ = true;
            direction_ = Direction::e_down;
            time_span_.second = the_time;
            return {Status::e_accepted, std::nullopt};
        }
        time_span_.second = the_time;
        return {Status::e_reversal, MakeReversalColumnLog(Direction::e_down, top_ - (log_box_increment_ * reversal_boxes_), the_time)};
    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Chart::TryToExtendUp  ----- 


PF_Column::AddResult PF_Column::TryToExtendDownLog (const DprDecimal::DDecQuad& possible_value, tpt the_time)
{
    if (possible_value <= bottom_ - log_box_increment_)
    {
        int how_many_boxes = ((possible_value - bottom_) / log_box_increment_).ToIntTruncated();
        bottom_ += how_many_boxes * log_box_increment_;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }
    // look for a reversal 

    if (possible_value >= bottom_ + (log_box_increment_ * reversal_boxes_))
    {
        // look for one-step-back reversal first.

        if (reversal_boxes_ == 1)
        {
            if (top_ >= bottom_ + log_box_increment_)
            {
                time_span_.second = the_time;
                // can't do it as box is occupied.
                return {Status::e_reversal, MakeReversalColumnLog(Direction::e_up, bottom_ + log_box_increment_, the_time)};
            }
            int how_many_boxes = ((possible_value - top_) / log_box_increment_).ToIntTruncated();
            top_ += how_many_boxes * log_box_increment_;
            had_reversal_ = true;
            direction_ = Direction::e_up;
            time_span_.second = the_time;
            return {Status::e_accepted, std::nullopt};
        }
        time_span_.second = the_time;
        return {Status::e_reversal, MakeReversalColumnLog(Direction::e_up, bottom_ + (log_box_increment_ * reversal_boxes_), the_time)};
    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToExtendDown  ----- 

DprDecimal::DDecQuad PF_Column::RoundDownToNearestBox (const DprDecimal::DDecQuad& a_value) const
{
    DprDecimal::DDecQuad price_as_int;
    if (fractional_boxes_ == FractionalBoxes::e_integral)
    {
        price_as_int = a_value.ToIntTruncated();
    }
    else
    {
        price_as_int = a_value;
    }

    DprDecimal::DDecQuad result = DprDecimal::Mod(price_as_int, box_size_) * box_size_;
    return result;

}		// -----  end of method PF_Column::RoundDowntoNearestBox  ----- 

    
Json::Value PF_Column::ToJSON () const
{
    Json::Value result;

    result["start_at"] = time_span_.first.time_since_epoch().count();
    result["last_entry"] = time_span_.second.time_since_epoch().count();

    result["box_size"] = box_size_.ToStr();
    result["log_box_increment"] = log_box_increment_.ToStr();
    result["reversal_boxes"] = reversal_boxes_;
    result["bottom"] = bottom_.ToStr();
    result["top"] = top_.ToStr();

    switch(direction_)
    {
        case Direction::e_unknown:
            result["direction"] = "unknown";
            break;

        case Direction::e_down:
            result["direction"] = "down";
            break;

        case Direction::e_up:
            result["direction"] = "up";
            break;
    };

    switch(fractional_boxes_)
    {
        case FractionalBoxes::e_integral:
            result["fractional_boxes"] = "integral";
            break;

        case FractionalBoxes::e_fractional:
            result["fractional_boxes"] = "fractional";
            break;
    };

    switch(column_scale_)
    {
        case ColumnScale::e_arithmetic:
            result["column_scale"] = "arithmetic";
            break;

        case ColumnScale::e_logarithmic:
            result["column_scale"] = "logarithmic";
            break;
    };

    switch(boxsize_percent_)
    {
        case BoxSizePerCent::e_scalar:
            result["boxsize_percent"] = "scalar";
            break;

        case BoxSizePerCent::e_percent:
            result["boxsize_percent"] = "percent";
            break;
    };

    result["had_reversal"] = had_reversal_;
    return result;
}		// -----  end of method PF_Column::ToJSON  ----- 

void PF_Column::FromJSON (const Json::Value& new_data)
{
    time_span_.first = tpt{std::chrono::nanoseconds{new_data["start_at"].asInt64()}};
    time_span_.second = tpt{std::chrono::nanoseconds{new_data["last_entry"].asInt64()}};

    box_size_ = DprDecimal::DDecQuad{new_data["box_size"].asString()};
    log_box_increment_ = DprDecimal::DDecQuad{new_data["log_box_increment"].asString()};
    reversal_boxes_ = new_data["reversal_boxes"].asInt();
    
    bottom_ = DprDecimal::DDecQuad{new_data["bottom"].asString()};
    top_ = DprDecimal::DDecQuad{new_data["top"].asString()};

    const auto direction = new_data["direction"].asString();
    if (direction == "up")
    {
        direction_ = Direction::e_up;
    }
    else if (direction == "down")
    {
        direction_ = Direction::e_down;
    }
    else if (direction == "unknown")
    {
        direction_ = Direction::e_unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid direction provided: {}. Must be 'up', 'down', 'unknown'.", direction)};
    }

    const auto fractional = new_data["fractional_boxes"].asString();
    if (fractional  == "integral")
    {

        fractional_boxes_ = FractionalBoxes::e_integral;
    }
    else if (fractional == "fractional")
    {
        fractional_boxes_ = FractionalBoxes::e_fractional;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid fractional_boxes provided: {}. Must be 'integral' or 'fractional'.", fractional)};
    }

    const auto column_scale = new_data["column_scale"].asString();
    if (column_scale  == "arithmetic")
    {
        column_scale_ = ColumnScale::e_arithmetic;
    }
    else if (column_scale == "logarithmic")
    {
        column_scale_ = ColumnScale::e_logarithmic;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid column_scale provided: {}. Must be 'arithmetic' or 'logarithmic'.", column_scale)};
    }

    const auto boxsize_percent = new_data["boxsize_percent"].asString();
    if (boxsize_percent  == "scalar")
    {
        boxsize_percent_ = BoxSizePerCent::e_scalar;
    }
    else if (boxsize_percent == "percent")
    {
        boxsize_percent_ = BoxSizePerCent::e_percent;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid boxsize_percent provided: {}. Must be 'scalar' or 'percent'.", boxsize_percent)};
    }

    had_reversal_ = new_data["had_reversal"].asBool();
    
}		// -----  end of method PF_Column::FromJSON  ----- 

