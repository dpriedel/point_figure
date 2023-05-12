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


	/* This file is part of PF_CollectData. */

	/* PF_CollectData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* PF_CollectData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with PF_CollectData.  If not, see <http://www.gnu.org/licenses/>. */

//-----------------------------------------------------------------------------
// 
// Logic for how to construct a column given incoming data comes from the
// excellent book "The Definitive Guide to Point and Figure" by
// Jeremy du Plessis.
// This book is written by an engineer/programmer so the alogorithm descriptions
// can be readily used to write code.  However, the implementation along with 
// responsibility for any errors is mine.
//
//-----------------------------------------------------------------------------

#include <exception>
#include <memory>


#include "Boxes.h"
#include "DDecQuad.h"
#include "PF_Column.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Column::PF_Column(Boxes* boxes, int32_t column_number, int32_t reversal_boxes,
            Direction direction, DprDecimal::DDecQuad top, DprDecimal::DDecQuad bottom)
    : boxes_{boxes}, column_number_{column_number}, reversal_boxes_{reversal_boxes}, top_{top}, bottom_{bottom}, direction_{direction}
{
}  // -----  end of method PF_Column::PF_Column  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Column::PF_Column (Boxes* boxes, const Json::Value& new_data)
    : boxes_{boxes}
{
    try
    {
        bool x = new_data.isMember("direction");
    }
    catch (Json::Exception& e)
    {
        throw std::domain_error{"Expected actual JSON data. Got something else."};
    }
    this->FromJSON(new_data);
}  // -----  end of method PF_Column::PF_Column  (constructor)  ----- 

PF_Column PF_Column::MakeReversalColumn (Direction direction, const DprDecimal::DDecQuad& value,
        TmPt the_time)
{
    auto new_column = PF_Column{boxes_, column_number_ + 1, reversal_boxes_, direction, value, value};
    new_column.time_span_ = {the_time, the_time};
    return new_column;
}		// -----  end of method PF_Column::MakeReversalColumn  ----- 

bool PF_Column::operator== (const PF_Column& rhs) const
{
    return rhs.reversal_boxes_ == reversal_boxes_ && rhs.direction_ == direction_
        && rhs.top_ == top_ && rhs.bottom_ == bottom_ && rhs.had_reversal_ == had_reversal_;
}		// -----  end of method PF_Column::operator==  ----- 

PF_Column::AddResult PF_Column::AddValue (const DprDecimal::DDecQuad& new_value, TmPt the_time)
{
    if (IsEmpty())
    {
        // OK, first time here for this column.

        return StartColumn(new_value, the_time);
    }

    // OK, we've got a value but may not yet have a direction.

    if (direction_ == Direction::e_unknown)
    {
        return TryToFindDirection(new_value, the_time);
    }

    // If we're here, we have direction. We can either continue in 
    // that direction, ignore the value or reverse our direction 
    // in which case, we start a new column (unless this is 
    // a 1-box reversal and we can reverse in place)

    if (direction_ == Direction::e_up)
    {
        return TryToExtendUp(new_value, the_time);
    }
    return TryToExtendDown(new_value, the_time);
}		// -----  end of method PF_Column::AddValue  ----- 

PF_Column::AddResult PF_Column::StartColumn (const DprDecimal::DDecQuad& new_value, TmPt the_time)
{
    // As this is the first entry in the column, just set fields 
    // to the input value rounded down to the nearest box value.

    top_= boxes_->FindBox(new_value);
    bottom_ = top_;
    time_span_ = {the_time, the_time};

    return {Status::e_accepted, std::nullopt};
}		// -----  end of method PF_Column::StartColumn  ----- 


PF_Column::AddResult PF_Column::TryToFindDirection (const DprDecimal::DDecQuad& new_value, TmPt the_time)
{
    // NOTE: Since a new value may gap up or down, we could 
    // have multiple boxes to fill in. 

    // we can compare to either value since they 
    // are both the same at this point.

    Boxes::Box possible_value = boxes_->FindBox(new_value);

    if (possible_value > top_)
    {
        direction_ = Direction::e_up;
        top_ = possible_value;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }
    if (possible_value < bottom_)
    {
        direction_ = Direction::e_down;
        bottom_ = possible_value;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }

    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToFindDirection  ----- 

PF_Column::AddResult PF_Column::TryToExtendUp (const DprDecimal::DDecQuad& new_value, TmPt the_time)
{
    // if we are going to extend the column up, then we need to move up by at least 1 box.

    Boxes::Box possible_new_top = boxes_->FindNextBox(top_);
    if (new_value >= possible_new_top)
    {
        // OK, up we go...

        while (possible_new_top <= new_value)
        {
            top_ = possible_new_top;
            possible_new_top = boxes_->FindNextBox(top_);
        }

        return {Status::e_accepted, std::nullopt};
    }

    // look for a reversal down 

    Boxes::Box possible_new_column_top = boxes_->FindPrevBox(top_);

    for (auto x = reversal_boxes_; x > 1; --x) 
    {
        possible_new_column_top = boxes_->FindPrevBox(possible_new_column_top);
    }

    if (new_value <= possible_new_column_top)
    {
        // look for 1-step back reversal.

        if (reversal_boxes_ == 1)
        {
            if (bottom_ == top_)
            {
                // OK, down we go with in-column reversal...

                bottom_ = possible_new_column_top;      // from loop above
                had_reversal_ = true;
                direction_ = Direction::e_down;
                time_span_.second = the_time;
                return {Status::e_accepted, std::nullopt};
            }
        }

        time_span_.second = the_time;
        return {Status::e_reversal, MakeReversalColumn(Direction::e_down, boxes_->FindPrevBox(top_), the_time)};
    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToExtendUp  ----- 

PF_Column::AddResult PF_Column::TryToExtendDown (const DprDecimal::DDecQuad& new_value, TmPt the_time)
{
    // if we are going to extend the column down, then we need to move down by at least 1 box.

    Boxes::Box possible_new_bottom = boxes_->FindPrevBox(bottom_);
    if (new_value <= possible_new_bottom)
    {
        // OK, down we go...

        while (possible_new_bottom >= new_value)
        {
            bottom_ = possible_new_bottom;
            possible_new_bottom = boxes_->FindPrevBox(bottom_);
        }

        return {Status::e_accepted, std::nullopt};
    }

    // look for a reversal up 

    Boxes::Box possible_new_column_bottom = boxes_->FindNextBox(bottom_);

    for (auto x = reversal_boxes_; x > 1; --x) 
    {
        possible_new_column_bottom = boxes_->FindNextBox(possible_new_column_bottom);
    }

    if (new_value >= possible_new_column_bottom)
    {
        // look for 1-step back reversal.

        if (reversal_boxes_ == 1)
        {
            if (bottom_ == top_)
            {
                // OK, up we go with in-column reversal...

                top_ = possible_new_column_bottom;      // from loop above
                had_reversal_ = true;
                direction_ = Direction::e_up;
                time_span_.second = the_time;
                return {Status::e_accepted, std::nullopt};
            }
        }

        time_span_.second = the_time;
        return {Status::e_reversal, MakeReversalColumn(Direction::e_up, boxes_->FindNextBox(bottom_), the_time)};
    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToExtendDown  ----- 

Json::Value PF_Column::ToJSON () const
{
    Json::Value result;

    result["first_entry"] = time_span_.first.time_since_epoch().count();
    result["last_entry"] = time_span_.second.time_since_epoch().count();

    result["column_number"] = column_number_;
    result["reversal_boxes"] = reversal_boxes_;
    result["top"] = top_.ToStr();
    result["bottom"] = bottom_.ToStr();

    switch(direction_)
    {
        using enum Direction;
        case e_unknown:
            result["direction"] = "unknown";
            break;

        case e_down:
            result["direction"] = "down";
            break;

        case e_up:
            result["direction"] = "up";
            break;
    };

    result["had_reversal"] = had_reversal_;
    return result;
}		// -----  end of method PF_Column::ToJSON  ----- 

void PF_Column::FromJSON (const Json::Value& new_data)
{
    time_span_.first = TmPt{std::chrono::utc_clock::duration{new_data["first_entry"].asInt64()}};
    time_span_.second = TmPt{std::chrono::utc_clock::duration{new_data["last_entry"].asInt64()}};

    column_number_ = new_data["column_number"].asInt();
    reversal_boxes_ = new_data["reversal_boxes"].asInt();
    top_ = DprDecimal::DDecQuad{new_data["top"].asString()};
    bottom_ = DprDecimal::DDecQuad{new_data["bottom"].asString()};

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
        throw std::invalid_argument{std::format("Invalid direction provided: {}. Must be 'up', 'down', 'unknown'.", direction)};
    }

    had_reversal_ = new_data["had_reversal"].asBool();
    
}		// -----  end of method PF_Column::FromJSON  ----- 

