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

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <utility>

namespace rng = std::ranges;

#include "Boxes.h"
#include "utilities.h"

//--------------------------------------------------------------------------------------
//       Class:  Boxes
//      Method:  Boxes
// Description:  constructor
//--------------------------------------------------------------------------------------
Boxes::Boxes(decimal::Decimal base_box_size, decimal::Decimal box_size_modifier, BoxScale box_scale)
    : base_box_size_{std::move(base_box_size)},
      box_size_modifier_{std::move(box_size_modifier)},
      box_type_{BoxType::e_Fractional},
      box_scale_{box_scale}
{
    if (base_box_size_.exponent() < kMinExponent)
    {
        base_box_size_ = base_box_size_.rescale(kMinExponent);
    }
    // std::print("params at Boxes start: {}, {}\n", base_box_size_, box_size_modifier_);
    runtime_box_size_ = base_box_size_;
    // std::print("params at Boxes start2: {}, {}, {}\n", base_box_size_, box_size_modifier_, runtime_box_size_);

    if (box_size_modifier_ != decimal::Decimal(0))
    {
        runtime_box_size_ = (base_box_size_ * box_size_modifier_)
                                .rescale(std::max(base_box_size_.exponent(), box_size_modifier_.exponent()) - 1);

        // it seems that the rescaled box size value can turn out to be zero. If that
        // is the case, then go with the unscaled box size.

        if (runtime_box_size_ == decimal::Decimal(0))
        {
            runtime_box_size_ = (base_box_size_ * box_size_modifier_).rescale(kMinExponent);
        }

        else  // percent box size
        {
            percent_box_factor_up_ =
                (decimal::Decimal{1} + box_size_modifier_)
                    .rescale(std::max(base_box_size_.exponent(), box_size_modifier_.exponent()) - 1);
            percent_box_factor_down_ =
                (decimal::Decimal{1} - box_size_modifier_)
                    .rescale(std::max(base_box_size_.exponent(), box_size_modifier_.exponent()) - 1);
            percent_exponent_ = percent_box_factor_up_.exponent();
        }
    }
    else
    {
        if (box_scale_ == BoxScale::e_Percent)
        {
            percent_box_factor_up_ = (decimal::Decimal{1} + base_box_size_);
            percent_box_factor_down_ = (decimal::Decimal{1} - base_box_size_);
            percent_exponent_ = base_box_size_.exponent() - 1;
        }
    }

    // try to keep box size from being too small

    if (runtime_box_size_.exponent() < -3)
    {
        runtime_box_size_ = runtime_box_size_.rescale(-3);
    }

    // we rarely need integral box types.

    if (runtime_box_size_.exponent() >= 0)
    {
        box_type_ = BoxType::e_Integral;
    }

}  // -----  end of method Boxes::Boxes  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  Boxes
//      Method:  Boxes
// Description:  constructor
//--------------------------------------------------------------------------------------
Boxes::Boxes(const Json::Value& new_data)
{
    this->FromJSON(new_data);
}  // -----  end of method Boxes::Boxes  (constructor)  -----

size_t Boxes::Distance(const Box& from, const Box& to) const
{
    if (from == to)
    {
        return 0;
    }

    const auto x = rng::find(boxes_, from);
    BOOST_ASSERT_MSG(x != boxes_.end(), "Can't find 'from' box in list.");
    const auto y = rng::find(boxes_, to);
    BOOST_ASSERT_MSG(y != boxes_.end(), "Can't find 'to' box in list.");

    if (from < to)
    {
        return rng::distance(x, y);
    }
    return rng::distance(y, x);
}  // -----  end of method Boxes::Distance  -----

Boxes::Box Boxes::FindBox(const decimal::Decimal& new_value)
{
    if (boxes_.empty())
    {
        return FirstBox(new_value);
    }

    if (box_scale_ == BoxScale::e_Percent)
    {
        return FindBoxPercent(new_value);
    }

    auto box_finder = [&new_value](const auto& a, const auto& b) { return new_value >= a && new_value < b; };

    // this code will not match against the last value in the list

    if (boxes_.size() > 1)
    {
        if (auto found_it = rng::adjacent_find(boxes_, box_finder); found_it != boxes_.end())
        {
            return *found_it;
        }

        if (new_value == boxes_.back())
        {
            return boxes_.back();
        }
    }
    // may have to extend box list by multiple boxes

    Box prev_back = boxes_.back();
    if (prev_back < new_value)
    {
        while (boxes_.back() < new_value)
        {
            // extend up

            prev_back = boxes_.back();
            Box new_box = prev_back + runtime_box_size_;
            PushBack(std::move(new_box));
        }
        return (new_value < boxes_.back() ? prev_back : boxes_.back());
    }

    // extend down

    while (new_value < boxes_.front())
    {
        Box new_box = boxes_.front() - runtime_box_size_;
        PushFront(new_box);
    };

    return boxes_.front();
}  // -----  end of method Boxes::FindBox  -----

Boxes::Box Boxes::FindBoxPercent(const decimal::Decimal& new_value)
{
    auto box_finder = [&new_value](const auto& a, const auto& b) { return new_value >= a && new_value < b; };

    // this code will not match against the last value in the list

    if (boxes_.size() > 1)
    {
        if (auto found_it = rng::adjacent_find(boxes_, box_finder); found_it != boxes_.end())
        {
            return *found_it;
        }

        if (new_value == boxes_.back())
        {
            return boxes_.back();
        }
    }
    // may have to extend box list by multiple boxes

    Box prev_back = boxes_.back();
    if (prev_back < new_value)
    {
        while (boxes_.back() < new_value)
        {
            // extend up

            prev_back = boxes_.back();
            Box new_box = (boxes_.back() * percent_box_factor_up_).rescale(percent_exponent_);
            // stocks trade in pennies, so minimum difference is $0.01
            if (new_box - boxes_.back() < sv2dec(".01"))
            {
                new_box = boxes_.back() + sv2dec(".01");
            }
            PushBack(std::move(new_box));
        }
        return (new_value < boxes_.back() ? prev_back : boxes_.back());
    }

    // extend down

    while (new_value < boxes_.front())
    {
        Box new_box = (boxes_.front() * percent_box_factor_down_).rescale(percent_exponent_);
        // stocks trade in pennies, so minimum difference is $0.01
        if (boxes_.front() - new_box < sv2dec(".01"))
        {
            new_box = boxes_.front() - sv2dec(".01");
        }
        PushFront(new_box);
    };

    return boxes_.front();
}  // -----  end of method Boxes::FindBox  -----

Boxes::Box Boxes::FindNextBox(const decimal::Decimal& current_value)
{
    BOOST_ASSERT_MSG(current_value >= boxes_.front() && current_value <= boxes_.back(),
                     std::format("Current value: {} is not contained in boxes.", current_value.format("f")).c_str());

    if (box_scale_ == BoxScale::e_Percent)
    {
        return FindNextBoxPercent(current_value);
    }

    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    // this code will not match against the last value in the list
    // which is OK since that means there will be no next box and the
    // index operator below will throw.

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    if (found_it == boxes_.end())
    {
        if (current_value == boxes_.back())
        {
            Box new_box = boxes_.back() + runtime_box_size_;
            PushBack(new_box);
            return new_box;
        }
    }

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    return boxes_.at(box_index + 1);
}  // -----  end of method Boxes::FindNextBox  -----

Boxes::Box Boxes::FindNextBox(const decimal::Decimal& current_value) const
{
    BOOST_ASSERT_MSG(current_value >= boxes_.front() && current_value <= boxes_.back(),
                     std::format("Current value: {} is not contained in boxes.", current_value.format("f")).c_str());

    if (box_scale_ == BoxScale::e_Percent)
    {
        return FindNextBoxPercent(current_value);
    }

    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    // this code will not match against the last value in the list
    // which is OK since that means there will be no next box and the
    // index operator below will throw.

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    BOOST_ASSERT_MSG(found_it != boxes_.end(),
                     std::format("Lookup-only box search failed for: ", current_value.format("f")).c_str());

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    return boxes_.at(box_index + 1);
}  // -----  end of method Boxes::FindNextBox  -----

Boxes::Box Boxes::FindNextBoxPercent(const decimal::Decimal& current_value)
{
    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    // this code will not match against the last value in the list
    // which is OK since that means there will be no next box and the
    // index operator below will throw.

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    if (found_it == boxes_.end())
    {
        if (current_value == boxes_.back())
        {
            Box new_box = (boxes_.back() * percent_box_factor_up_).rescale(percent_exponent_);
            // stocks trade in pennies, so minimum difference is $0.01
            if (new_box - boxes_.back() < sv2dec(".01"))
            {
                new_box = boxes_.back() + sv2dec(".01");
            }
            PushBack(new_box);
            return new_box;
        }
    }

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    return boxes_.at(box_index + 1);
}  // -----  end of method Boxes::FindNextBoxPercent  -----

Boxes::Box Boxes::FindNextBoxPercent(const decimal::Decimal& current_value) const
{
    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    // this code will not match against the last value in the list
    // which is OK since that means there will be no next box and the
    // index operator below will throw.

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    BOOST_ASSERT_MSG(found_it != boxes_.end(),
                     std::format("Lookup-only box search failed for: ", current_value.format("f")).c_str());

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    return boxes_.at(box_index + 1);
}  // -----  end of method Boxes::FindNextBoxPercent  -----

Boxes::Box Boxes::FindPrevBox(const decimal::Decimal& current_value)
{
    BOOST_ASSERT_MSG(current_value >= boxes_.front() && current_value <= boxes_.back(),
                     std::format("Current value: {} is not contained in boxes.", current_value.format("f")).c_str());

    if (box_scale_ == BoxScale::e_Percent)
    {
        return FindPrevBoxPercent(current_value);
    }

    if (boxes_.size() == 1)
    {
        Box new_box = boxes_.front() - runtime_box_size_;
        PushFront(new_box);
        return new_box;
    }

    // this code will not match against the last value in the list

    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    if (found_it == boxes_.end())
    {
        if (current_value == boxes_.back())
        {
            return boxes_.back();
        }
    }

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    if (box_index == 0)
    {
        Box new_box = boxes_.front() - runtime_box_size_;
        PushFront(new_box);
        return boxes_.front();
    }
    return boxes_.at(box_index - 1);
}  // -----  end of method Boxes::FindPrevBox  -----

Boxes::Box Boxes::FindPrevBox(const decimal::Decimal& current_value) const
{
    BOOST_ASSERT_MSG(
        current_value > boxes_.front() && current_value <= boxes_.back(),
        std::format("Lookup-only search for previous box for value: {} failed.", current_value.format("f")).c_str());

    if (box_scale_ == BoxScale::e_Percent)
    {
        return FindPrevBoxPercent(current_value);
    }

    // this code will not match against the last value in the list

    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    if (found_it == boxes_.end())
    {
        if (current_value == boxes_.back())
        {
            return boxes_.back();
        }
    }

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    BOOST_ASSERT_MSG(box_index > 0,
                     std::format("Lookup-only box search failed for: ", current_value.format("f")).c_str());
    return boxes_.at(box_index - 1);
}  // -----  end of method Boxes::FindPrevBox  -----

Boxes::Box Boxes::FindPrevBoxPercent(const decimal::Decimal& current_value)
{
    if (boxes_.size() == 1)
    {
        Box new_box = (boxes_.front() * percent_box_factor_down_).rescale(percent_exponent_);
        // stocks trade in pennies, so minimum difference is $0.01
        if (boxes_.front() - new_box < sv2dec(".01"))
        {
            new_box = boxes_.front() - sv2dec(".01");
        }
        PushFront(new_box);
        return new_box;
    }

    // this code will not match against the last value in the list

    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    if (found_it == boxes_.end())
    {
        if (current_value == boxes_.back())
        {
            return boxes_.at(boxes_.size() - 2);
        }
    }

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    if (box_index == 0)
    {
        Box new_box = (boxes_.front() * percent_box_factor_down_).rescale(percent_exponent_);
        // stocks trade in pennies, so minimum difference is $0.01
        if (boxes_.front() - new_box < sv2dec(".01"))
        {
            new_box = boxes_.front() - sv2dec(".01");
        }
        PushFront(new_box);
        return boxes_.front();
    }
    return boxes_.at(box_index - 1);
}  // -----  end of method Boxes::FindNextBoxPercent  -----

Boxes::Box Boxes::FindPrevBoxPercent(const decimal::Decimal& current_value) const
{
    // this code will not match against the last value in the list

    auto box_finder = [&current_value](const auto& a, const auto& b)
    { return current_value >= a && current_value < b; };

    auto found_it = rng::adjacent_find(boxes_, box_finder);
    if (found_it == boxes_.end())
    {
        if (current_value == boxes_.back())
        {
            return boxes_.at(boxes_.size() - 2);
        }
    }

    size_t box_index = rng::distance(boxes_.begin(), found_it);
    BOOST_ASSERT_MSG(box_index > 0,
                     std::format("Lookup-only box search failed for: ", current_value.format("f")).c_str());
    return boxes_.at(box_index - 1);
}  // -----  end of method Boxes::FindNextBoxPercent  -----

Boxes& Boxes::operator=(const Json::Value& new_data)
{
    this->FromJSON(new_data);
    return *this;
}  // -----  end of method Boxes::operator=  -----

bool Boxes::operator==(const Boxes& rhs) const
{
    if (rhs.base_box_size_ != base_box_size_)
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
}  // -----  end of method Boxes::operator==  -----

Boxes::Box Boxes::FirstBox(const decimal::Decimal& start_at)
{
    BOOST_ASSERT_MSG(base_box_size_ != -1, "'box_size' must be specified before adding boxes_.");

    //    if (box_scale_ == BoxScale::e_percent)
    //    {
    //        return FirstBoxPerCent(start_at);
    //    }
    boxes_.clear();

    decimal::Decimal price_as_int_or_not;
    if (box_type_ == BoxType::e_Integral)
    {
        price_as_int_or_not = start_at.to_integral();
    }
    else
    {
        price_as_int_or_not = start_at;
    }

    //    auto new_box = RoundDownToNearestBox(start_at);
    Box new_box{price_as_int_or_not};
    PushBack(new_box);
    return new_box;

}  // -----  end of method Boxes::NewBox  -----

Boxes::Box Boxes::FirstBoxPerCent(const decimal::Decimal& start_at)
{
    BOOST_ASSERT_MSG(base_box_size_ != -1, "'box_size' must be specified before adding boxes_.");

    boxes_.clear();
    //    auto new_box = RoundDownToNearestBox(start_at);
    Box new_box{start_at};
    PushBack(new_box);
    return new_box;

}  // -----  end of method Boxes::NewBox  -----

Boxes::Box Boxes::RoundDownToNearestBox(const decimal::Decimal& a_value) const
{
    decimal::Decimal price_as_int;
    if (box_type_ == BoxType::e_Integral)
    {
        price_as_int = a_value.to_integral();
    }
    else
    {
        price_as_int = a_value;
    }

    Box result = price_as_int.divmod(base_box_size_).first * base_box_size_;
    return result;

}  // -----  end of method PF_Column::RoundDowntoNearestBox  -----

Json::Value Boxes::ToJSON() const
{
    Json::Value result;

    result["box_size"] = base_box_size_.format("f");
    result["box_size_modifier"] = box_size_modifier_.format("f");
    result["runtime_box_size"] = runtime_box_size_.format("f");
    result["factor_up"] = percent_box_factor_up_.format("f");
    result["factor_down"] = percent_box_factor_down_.format("f");
    result["exponent"] = percent_exponent_;

    switch (box_type_)
    {
        case BoxType::e_Integral:
            result["box_type"] = "integral";
            break;

        case BoxType::e_Fractional:
            result["box_type"] = "fractional";
            break;
    };

    switch (box_scale_)
    {
        case BoxScale::e_Linear:
            result["box_scale"] = "linear";
            break;

        case BoxScale::e_Percent:
            result["box_scale"] = "percent";
            break;
    };

    Json::Value the_boxes{Json::arrayValue};
    for (const auto& box : boxes_)
    {
        the_boxes.append(box.format("f"));
    }
    result["boxes"] = the_boxes;

    return result;
}  // -----  end of method Boxes::FromJSON  -----

void Boxes::FromJSON(const Json::Value& new_data)
{
    base_box_size_ = decimal::Decimal{new_data["box_size"].asCString()};
    box_size_modifier_ = decimal::Decimal{new_data["box_size_modifier"].asCString()};
    runtime_box_size_ = decimal::Decimal{new_data["runtime_box_size"].asCString()};
    percent_box_factor_up_ = decimal::Decimal{new_data["factor_up"].asCString()};
    percent_box_factor_down_ = decimal::Decimal{new_data["factor_down"].asCString()};
    percent_exponent_ = new_data["exponent"].asInt();

    const auto box_type = new_data["box_type"].asString();
    if (box_type == "integral")
    {
        box_type_ = BoxType::e_Integral;
    }
    else if (box_type == "fractional")
    {
        box_type_ = BoxType::e_Fractional;
    }
    else
    {
        throw std::invalid_argument{
            std::format("Invalid box_type provided: {}. Must be 'integral' or 'fractional'.", box_type)};
    }

    const auto box_scale = new_data["box_scale"].asString();
    if (box_scale == "linear")
    {
        box_scale_ = BoxScale::e_Linear;
    }
    else if (box_scale == "percent")
    {
        box_scale_ = BoxScale::e_Percent;
    }
    else
    {
        throw std::invalid_argument{
            std::format("Invalid box scale provided: {}. Must be 'linear' or 'percent'.", box_scale)};
    }

    // lastly, we can do our boxes

    const auto& the_boxes = new_data["boxes"];
    boxes_.clear();
    rng::for_each(the_boxes, [this](const auto& next_box) { this->boxes_.emplace_back(next_box.asCString()); });

    // we expect these values to be in ascending order, so let'ts make sure

    auto x = rng::adjacent_find(boxes_, rng::greater());
    BOOST_ASSERT_MSG(x == boxes_.end(), "boxes must be in ascending order and it isn't.");
}  // -----  end of method Boxes::FromJSON  -----

void Boxes::PushFront(Box new_box)
{
    BOOST_ASSERT_MSG(
        boxes_.size() < kMaxBoxes,
        std::format("Maximum number of boxes ({}) reached. Use a box size larger than: {}. [{}, {}, {}, {}, {}]",
                    kMaxBoxes, base_box_size_.format("f"), boxes_[0].format("f"), boxes_[1].format("f"),
                    boxes_[2].format("f"), boxes_[3].format("f"), boxes_[4].format("f"))
            .c_str());
    boxes_.insert(boxes_.begin(), std::move(new_box));

}  // -----  end of method Boxes::PushFront  -----

void Boxes::PushBack(Box new_box)
{
    BOOST_ASSERT_MSG(
        boxes_.size() < kMaxBoxes,
        std::format("Maximum number of boxes ({}) reached. Use a box size larger than: {}. [{}, {}, {}, {}, {}]",
                    kMaxBoxes, base_box_size_.format("f"), boxes_[kMaxBoxes - 5].format("f"),
                    boxes_[kMaxBoxes - 4].format("f"), boxes_[kMaxBoxes - 3].format("f"),
                    boxes_[kMaxBoxes - 2].format("f"), boxes_[kMaxBoxes - 1].format("f"))
            .c_str());
    boxes_.push_back(std::move(new_box));
}  // -----  end of method Boxes::PushBack  -----
