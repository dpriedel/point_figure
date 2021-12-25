// =====================================================================================
//
//       Filename:  boxes_.cpp
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

#include <cstdint>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/adjacent_find.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/for_each.hpp>


#include "Boxes.h"
#include "utilities.h"

//--------------------------------------------------------------------------------------
//       Class:  Boxes
//      Method:  Boxes
// Description:  constructor
//--------------------------------------------------------------------------------------
Boxes::Boxes (DprDecimal::DDecQuad box_size, BoxType box_type, BoxScale box_scale)
    : box_size_{box_size}, box_type_{box_type}, box_scale_{box_scale}
{
    if (box_scale_ == BoxScale::e_percent)
    {
        percent_box_factor_up_ = (1.0 + box_size_);
        percent_exponent_ = box_size_.GetExponent() - 1;
        percent_box_factor_down_ = (box_size_ / percent_box_factor_up_).Rescale(percent_exponent_);
    }
}  // -----  end of method Boxes::Boxes  (constructor)  ----- 

//--------------------------------------------------------------------------------------
//       Class:  Boxes
//      Method:  Boxes
// Description:  constructor
//--------------------------------------------------------------------------------------
Boxes::Boxes (const Json::Value& new_data)
{
    this->FromJSON(new_data);
}  // -----  end of method Boxes::Boxes  (constructor)  ----- 

int32_t Boxes::Distance (const Box& from, const Box& to) const
{
    if (from == to)
    {
        return 0;
    }
    
    const auto x = ranges::find(boxes_, from);
    BOOST_ASSERT_MSG(x != boxes_.end(), "Can't find 'from' box in list.");
    const auto y = ranges::find(boxes_, to);
    BOOST_ASSERT_MSG(y != boxes_.end(), "Can't find 'to' box in list.");

    if (from < to)
    {
        return ranges::distance(x, y);
    }
    return ranges::distance(y, x);
}		// -----  end of method Boxes::Distance  ----- 


Boxes::Box Boxes::FindBox (const DprDecimal::DDecQuad& new_value)
{
    if (boxes_.empty())
    {
        return FirstBox(new_value);
    }

    if (box_scale_ == BoxScale::e_percent)
    {
        return FindBoxPercent(new_value);
    }

    auto box_finder = [&new_value](const auto& a, const auto& b) { return new_value >= a && new_value < b; };

    // this code will not match against the last value in the list 

    if (auto found_it = ranges::adjacent_find(boxes_, box_finder); found_it != boxes_.end())
    {
        return *found_it;
    }

    if (new_value == boxes_.back())
    {
        return boxes_.back();
    }
    // may have to extend box list by multiple boxes 

    if (new_value > boxes_.back())
    {
        // extend up 

        Box prev_box = boxes_.back();
        Box new_box = prev_box + box_size_;

        do
        {
            prev_box = boxes_.back();
            boxes_.push_back(new_box);
            new_box += box_size_;
        } while (new_value >= boxes_.back());
        return prev_box;
    }
    else
    {
        // extend down 
       
        do 
        {
            Box new_box = boxes_.front() - box_size_;
            boxes_.push_front(new_box);
        } while (new_value < boxes_.front());

        return boxes_.front();
    }
    return {};
}		// -----  end of method Boxes::FindBox  ----- 

Boxes::Box Boxes::FindBoxPercent (const DprDecimal::DDecQuad& new_value)
{
    auto box_finder = [&new_value](const auto& a, const auto& b) { return new_value >= a && new_value < b; };

    // this code will not match against the last value in the list 

    if (auto found_it = ranges::adjacent_find(boxes_, box_finder); found_it != boxes_.end())
    {
        return *found_it;
    }

    if (new_value == boxes_.back())
    {
        return boxes_.back();
    }
    // may have to extend box list by multiple boxes 

    if (new_value > boxes_.back())
    {
        // extend up 

        Box prev_box = boxes_.back();
        Box new_box = (prev_box * percent_box_factor_up_).Rescale(percent_exponent_);

        do
        {
            prev_box = boxes_.back();
            boxes_.push_back(new_box);
            new_box = (new_box * percent_box_factor_up_).Rescale(percent_exponent_);
        } while (new_value >= boxes_.back());
        return prev_box;
    }
    else
    {
        // extend down 
       
        do 
        {
            Box new_box = (boxes_.front() * percent_box_factor_down_).Rescale(percent_exponent_);
            boxes_.push_front(new_box);
        } while (new_value < boxes_.front());

        return boxes_.front();
    }
    return {};
}		// -----  end of method Boxes::FindBox  ----- 

Boxes::Box Boxes::FindNextBox (const DprDecimal::DDecQuad& current_value)
{
    auto box_finder = [&current_value](const auto& a, const auto& b) { return current_value >= a && current_value < b; };

    // this code will not match against the last value in the list 
    // which is OK since that means there will be no next box and the
    // index operator below will throw.

    auto found_it = ranges::adjacent_find(boxes_, box_finder);
    BOOST_ASSERT_MSG(found_it != boxes_.end(), "Can't find 'current box' box in list.");

    int32_t box_index = ranges::distance(boxes_.begin(), found_it);
    return boxes_[box_index + 1];
}		// -----  end of method Boxes::FindNextBox  ----- 

Boxes::Box Boxes::FindPrevBox (const DprDecimal::DDecQuad& current_value)
{
    // this code will not match against the last value in the list 

    auto box_finder = [&current_value](const auto& a, const auto& b) { return current_value >= a && current_value < b; };

    if (current_value == boxes_.back())
    {
        int32_t box_index = boxes_.size() - 1;
        return boxes_[box_index - 1];
    }
    auto found_it = ranges::adjacent_find(boxes_, box_finder);
    BOOST_ASSERT_MSG(found_it != boxes_.end(), "Can't find 'current box' box in list.");

    int32_t box_index = ranges::distance(boxes_.begin(), found_it);
    return boxes_[box_index - 1];
}		// -----  end of method Boxes::FindPrevBox  ----- 

Boxes& Boxes::operator= (const Json::Value& new_data)
{
    this->FromJSON(new_data);
    return *this;
}		// -----  end of method Boxes::operator=  ----- 


bool Boxes::operator== (const Boxes& rhs) const
{
    if (rhs.box_size_ != box_size_)
    {
        return false;
    }

    if (rhs.box_type_ != box_type_)
    {
        return false;
    }
    if (rhs.box_scale_ != box_scale_)
    {
        return false;
    }
    return rhs.boxes_ == boxes_;
}		// -----  end of method Boxes::operator==  ----- 

Boxes::Box Boxes::FirstBox (const DprDecimal::DDecQuad& start_at)
{
    BOOST_ASSERT_MSG(box_size_ != -1, "'box_size' must be specified before adding boxes_.");

//    if (box_scale_ == BoxScale::e_percent)
//    {
//        return FirstBoxPerCent(start_at);
//    }
    boxes_.clear();
    auto new_box = RoundDownToNearestBox(start_at);
    boxes_.push_back(new_box);
    return new_box;

}		// -----  end of method Boxes::NewBox  ----- 

Boxes::Box Boxes::FirstBoxPerCent (const DprDecimal::DDecQuad& start_at)
{
    BOOST_ASSERT_MSG(box_size_ != -1, "'box_size' must be specified before adding boxes_.");

    boxes_.clear();
    auto new_box = RoundDownToNearestBox(start_at);
    boxes_.push_back(new_box);
    return new_box;

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

Json::Value Boxes::ToJSON () const
{
    Json::Value result;

    result["box_size"] = box_size_.ToStr();
    result["factor_up"] = percent_box_factor_up_.ToStr();
    result["factor_down"] = percent_box_factor_down_.ToStr();
    result["exponent"] = percent_exponent_;

    switch(box_type_)
    {
        case BoxType::e_integral:
            result["box_type"] = "integral";
            break;

        case BoxType::e_fractional:
            result["box_type"] = "fractional";
            break;
    };

    switch(box_scale_)
    {
        case BoxScale::e_linear:
            result["box_scale"] = "linear";
            break;

        case BoxScale::e_percent:
            result["box_scale"] = "percent";
            break;
    };

    Json::Value the_boxes{Json::arrayValue};
    for (const auto& box : boxes_)
    {
        the_boxes.append(box.ToStr());
    }
    result["boxes"] = the_boxes;

    return result;
}		// -----  end of method Boxes::FromJSON  ----- 

void Boxes::FromJSON (const Json::Value& new_data)
{
    box_size_ = DprDecimal::DDecQuad{new_data["box_size"].asString()};
    percent_box_factor_up_ = DprDecimal::DDecQuad{new_data["factor_up"].asString()};
    percent_box_factor_down_ = DprDecimal::DDecQuad{new_data["factor_down"].asString()};
    percent_exponent_ = new_data["exponent"].asInt();

    const auto box_type = new_data["box_type"].asString();
    if (box_type  == "integral")
    {

        box_type_ = BoxType::e_integral;
    }
    else if (box_type == "fractional")
    {
        box_type_ = BoxType::e_fractional;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid box_type provided: {}. Must be 'integral' or 'fractional'.", box_type)};
    }

    const auto box_scale = new_data["box_scale"].asString();
    if (box_scale == "linear")
    {
        box_scale_ = BoxScale::e_linear;
    }
    else if (box_scale == "percent")
    {
        box_scale_ = BoxScale::e_percent;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid box scale provided: {}. Must be 'linear' or 'percent'.", box_scale)};
    }
    
    // lastly, we can do our boxes 

    const auto& the_boxes = new_data["boxes"];
    ranges::for_each(the_boxes, [this](const auto& next_box) { this->boxes_.emplace_back(next_box.asString()); });

}		// -----  end of method Boxes::FromJSON  ----- 

