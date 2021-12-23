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

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/adjacent_find.hpp>


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

Boxes::Box Boxes::FindBox (const DprDecimal::DDecQuad& new_value)
{
    if (boxes.empty())
    {
        return FirstBox(new_value);
    }

    if (box_scale_ == BoxScale::e_percent)
    {
        return FindBoxPercent(new_value);
    }

    auto box_finder = [&new_value](const auto& a, const auto& b) { return new_value >= a && new_value < b; };

    // this code will not match against the last value in the list 

    if (auto found_it = ranges::adjacent_find(boxes, box_finder); found_it != boxes.end())
    {
        return *found_it;
    }

    if (new_value == boxes.back())
    {
        return boxes.back();
    }
    // may have to extend box list by multiple boxes 

    if (new_value > boxes.back())
    {
        // extend up 

        Box prev_box = boxes.back();
        Box new_box = prev_box + box_size_;

        do
        {
            prev_box = boxes.back();
            boxes.push_back(new_box);
            new_box += box_size_;
        } while (new_value >= boxes.back());
        return prev_box;
    }
    else
    {
        // extend down 
       
        do 
        {
            Box new_box = boxes.front() - box_size_;
            boxes.push_front(new_box);
        } while (new_value < boxes.front());

        return boxes.front();
    }
    return {};
}		// -----  end of method Boxes::FindBox  ----- 

Boxes::Box Boxes::FindBoxPercent (const DprDecimal::DDecQuad& new_value)
{
    auto box_finder = [&new_value](const auto& a, const auto& b) { return new_value >= a && new_value < b; };

    // this code will not match against the last value in the list 

    if (auto found_it = ranges::adjacent_find(boxes, box_finder); found_it != boxes.end())
    {
        return *found_it;
    }

    if (new_value == boxes.back())
    {
        return boxes.back();
    }
    // may have to extend box list by multiple boxes 

    if (new_value > boxes.back())
    {
        // extend up 

        Box prev_box = boxes.back();
        Box new_box = (prev_box * percent_box_factor_up_).Rescale(percent_exponent_);

        do
        {
            prev_box = boxes.back();
            boxes.push_back(new_box);
            new_box = (new_box * percent_box_factor_up_).Rescale(percent_exponent_);
        } while (new_value >= boxes.back());
        return prev_box;
    }
    else
    {
        // extend down 
       
        do 
        {
            Box new_box = (boxes.front() * percent_box_factor_down_).Rescale(percent_exponent_);
            boxes.push_front(new_box);
        } while (new_value < boxes.front());

        return boxes.front();
    }
    return {};
}		// -----  end of method Boxes::FindBox  ----- 


Boxes::Box Boxes::FirstBox (const DprDecimal::DDecQuad& start_at)
{
    BOOST_ASSERT_MSG(box_size_ != -1, "'box_size' must be specified before adding boxes.");

//    if (box_scale_ == BoxScale::e_percent)
//    {
//        return FirstBoxPerCent(start_at);
//    }
    boxes.clear();
    auto new_box = RoundDownToNearestBox(start_at);
    boxes.push_back(new_box);
    return new_box;

}		// -----  end of method Boxes::NewBox  ----- 

Boxes::Box Boxes::FirstBoxPerCent (const DprDecimal::DDecQuad& start_at)
{
    BOOST_ASSERT_MSG(box_size_ != -1, "'box_size' must be specified before adding boxes.");

    boxes.clear();
    auto new_box = RoundDownToNearestBox(start_at);
    boxes.push_back(new_box);
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
