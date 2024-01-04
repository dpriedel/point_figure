// =====================================================================================
//
//       Filename:  PF_Chart.h
//
//    Description:  class to manage Point & Figure data for a symbol.
//
//        Version:  2.0
//        Created:  2021-07-29 08:29 AM
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

// =====================================================================================
//        Class:  PF_Chart
//  Description:  class to manage Point & Figure data for a symbol.
// =====================================================================================

#ifndef PF_CHART_INC
#define PF_CHART_INC

#include <json/json.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <decimal.hh>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "Boxes.h"
#include "PF_Column.h"
#include "PF_Signals.h"
#include "PointAndFigureDB.h"
#include "utilities.h"

// helpers for building chart graphics

enum class PF_ColumnFilter : char { e_up_column, e_down_column, e_reversed_to_up, e_reversed_to_down };

class PF_Chart
{
   public:
    class PF_Chart_Iterator;
    using iterator = PF_Chart_Iterator;
    using const_iterator = PF_Chart_Iterator;

    class PF_Chart_ReverseIterator;
    using reverse_iterator = PF_Chart_ReverseIterator;
    using const_reverse_iterator = PF_Chart_ReverseIterator;

   public:
    using Y_Limits = std::pair<decimal::Decimal, decimal::Decimal>;
    using PF_ChartParams = std::tuple<std::string, decimal::Decimal, int32_t, BoxScale>;

    enum class X_AxisFormat : char
    {
        e_show_date,
        e_show_time
    };
    enum
    {
        e_symbol = 0,
        e_box_size,
        e_reversal,
        e_box_scale
    };
    //
    //    using const_iterator = std::vector<PF_Column>::const_iterator;

    // ====================  LIFECYCLE =======================================
    PF_Chart() = default;  // constructor
    PF_Chart(const PF_Chart &rhs);
    PF_Chart(PF_Chart &&rhs) noexcept;

    PF_Chart(std::string symbol, decimal::Decimal base_box_size, int32_t reversal_boxes,
             decimal::Decimal box_size_modifier = 0, BoxScale box_scale = BoxScale::e_Linear,
             int64_t max_columns_for_graph = 0);

    PF_Chart(const PF_ChartParams &vals, decimal::Decimal box_size_modifier, int64_t max_columns_for_graph)
        : PF_Chart(std::get<e_symbol>(vals), std::get<e_box_size>(vals), std::get<e_reversal>(vals), box_size_modifier,
                   std::get<e_box_scale>(vals), max_columns_for_graph)
    {
    }

    explicit PF_Chart(const Json::Value &new_data);

    ~PF_Chart() = default;

    static PF_Chart MakeChartFromDB(const PF_DB &chart_db, PF_ChartParams vals, std::string_view interval);

    // mainly for Python wrapper
    static PF_Chart MakeChartFromJSONFile(const fs::path& file_name);

    // ====================  ACCESSORS =======================================

    [[nodiscard]] iterator begin();
    [[nodiscard]] const_iterator begin() const;
    [[nodiscard]] iterator end();
    [[nodiscard]] const_iterator end() const;

    [[nodiscard]] reverse_iterator rbegin();
    [[nodiscard]] const_reverse_iterator rbegin() const;
    [[nodiscard]] reverse_iterator rend();
    [[nodiscard]] const_reverse_iterator rend() const;

    [[nodiscard]] const PF_Column &front() const { return (*this)[0]; }
    [[nodiscard]] const PF_Column &back() const { return current_column_; }

    [[nodiscard]] bool empty() const { return columns_.empty() && current_column_.IsEmpty(); }
    [[nodiscard]] decimal::Decimal GetChartBoxSize() const { return boxes_.GetBoxSize(); }
    [[nodiscard]] decimal::Decimal GetFNameBoxSize() const { return fname_box_size_; }
    [[nodiscard]] int32_t GetReversalboxes() const { return current_column_.GetReversalboxes(); }
    [[nodiscard]] BoxScale GetBoxScale() const { return boxes_.GetBoxScale(); }
    [[nodiscard]] BoxType GetBoxType() const { return boxes_.GetBoxType(); }
    [[nodiscard]] std::string GetSymbol() const { return symbol_; }
    [[nodiscard]] std::string GetChartBaseName() const { return chart_base_name_; }
    [[nodiscard]] bool HasReversedColumns() const;

    // for Python

    [[nodiscard]] const PF_Column &GetColumn(size_t which) const { return (*this)[which]; }
    [[nodiscard]] PF_Column::Direction GetCurrentDirection() const { return current_direction_; }

    // if you know that a signal was just triggered, then this routine
    // will tell you what it was.
    [[nodiscard]] std::optional<PF_Signal> GetMostRecentSignal() const
    {
        return (signals_.empty() ? std::optional<PF_Signal>{std::nullopt} : signals_.back());
    }

    // if you just want to know whether there is a signal active
    // for the current column, then use this routine.

    [[nodiscard]] std::optional<PF_Signal> GetCurrentSignal() const
    {
        if (!signals_.empty())
        {
            if (const auto &sig = signals_.back(); sig.column_number_ == current_column_.GetColumnNumber())
            {
                return sig;
            }
        }
        return {};
    }
    // includes 'current_column'
    // [[nodiscard]] int32_t GetNumberOfColumns() const { return columns_.size()
    // + 1; }
    [[nodiscard]] size_t size() const { return columns_.size() + 1; }

    [[nodiscard]] Y_Limits GetYLimits() const { return {y_min_, y_max_}; }

    [[nodiscard]] PF_Column::TmPt GetFirstTime() const { return first_date_; }
    [[nodiscard]] PF_Column::TmPt GetLastChangeTime() const { return last_change_date_; }
    [[nodiscard]] PF_Column::TmPt GetLastCheckedTime() const { return last_checked_date_; }

    [[nodiscard]] std::string MakeChartFileName(std::string_view interval, std::string_view suffix) const;

    void ConvertChartToJsonAndWriteToFile(const fs::path &output_filename) const;
    void ConvertChartToJsonAndWriteToStream(std::ostream &stream) const;

    void ConvertChartToTableAndWriteToFile(const fs::path &output_filename,
                                           X_AxisFormat date_or_time = X_AxisFormat::e_show_date) const;
    void ConvertChartToTableAndWriteToStream(std::ostream &stream,
                                             X_AxisFormat date_or_time = X_AxisFormat::e_show_date) const;

    void StoreChartInChartsDB(const PF_DB &chart_db, std::string_view interval,
                              X_AxisFormat date_or_time = X_AxisFormat::e_show_date,
                              bool store_cvs_graphics = false) const;
    void UpdateChartInChartsDB(const PF_DB &chart_db, std::string_view interval,
                               X_AxisFormat date_or_time = X_AxisFormat::e_show_date,
                               bool store_cvs_graphics = false) const;

    [[nodiscard]] Json::Value ToJSON() const;
    [[nodiscard]] bool IsPercent() const { return boxes_.GetBoxScale() == BoxScale::e_Percent; }
    [[nodiscard]] bool IsFractional() const { return boxes_.GetBoxType() == BoxType::e_Fractional; }

    [[nodiscard]] const Boxes &GetBoxes() const { return boxes_; }
    [[nodiscard]] const PF_SignalList &GetSignals() const { return signals_; }

    // NOTE: this does NOT include current_column_ so in order to avoid confusion, remove it.
    // ** use the iterator interface to properly access columns **
    // [[nodiscard]] const std::vector<PF_Column> &GetColumns() const { return columns_; }
    // [[nodiscard]] const PF_Column& GetCurrentColumn() const { return
    // current_column_; }

    [[nodiscard]] PF_ChartParams GetChartParams() const
    {
        return {symbol_, fname_box_size_, current_column_.GetReversalboxes(), boxes_.GetBoxScale()};
    }

    // for drawing chart, boxes are needed in floating point format, not decimal

    using ColumnBoxList = std::vector<std::pair<int, double>>;

    // collect list of boxes and column numbers for all columns with the specified direction

    [[nodiscard]] ColumnBoxList GetBoxesForColumns(PF_ColumnFilter which_columns) const;

    // this routine supports drawing graphics using box-type charts.  To support this,
    // the value of column top is actually 1 box higher so that the graphic will be a
    // better representation of the column.
    // Returned values are also floats since that is what the graphic software wants.

    struct ColumnTopBottomInfo
    {
        int col_nbr_ = {};
        double col_top_ = {};
        double col_bot_ = {};
    };
    using ColumnTopBottomList = std::vector<ColumnTopBottomInfo>;

    [[nodiscard]] ColumnTopBottomList GetTopBottomForColumns(PF_ColumnFilter which_columns) const;

    // ====================  MUTATORS =======================================

    PF_Column::Status AddValue(const decimal::Decimal &new_value, PF_Column::TmPt the_time);
    PF_Column::Status AddValue(std::string_view new_value, std::string_view time_value, std::string_view time_format)
    {
        return AddValue(sv2dec(new_value), StringToUTCTimePoint(time_format, time_value));
    }
    // for Python - value as floating point and time in seconds
    PF_Column::Status AddValue(double new_value, int64_t the_time)
    {
        return AddValue(dbl2dec(new_value), PF_Column::TmPt{std::chrono::seconds(the_time)});
    }
    void LoadData(std::istream *input_data, std::string_view date_format, std::string_view delim);
    StreamedPrices LoadDataCollectPricesAndSignals(std::istream *input_data, std::string_view date_format, std::string_view delim);
    void LoadDataFromFile(const std::string &file_name, std::string_view date_format, std::string_view delim);

    [[nodiscard]] int64_t GetMaxGraphicColumns() const { return max_columns_for_graph_; }
    void SetMaxGraphicColumns(int64_t max_cols) { max_columns_for_graph_ = max_cols; }

    void AddSignal(const PF_Signal &new_sig) { signals_.push_back(new_sig); }

    // ====================  OPERATORS =======================================

    PF_Chart &operator=(const PF_Chart &rhs);
    PF_Chart &operator=(PF_Chart &&rhs) noexcept;

    PF_Chart &operator=(const Json::Value &new_data);

    bool operator==(const PF_Chart &rhs) const;
    bool operator!=(const PF_Chart &rhs) const { return !operator==(rhs); }

    const PF_Column &operator[](size_t which) const
    {
        const PF_Column &col = which < columns_.size() ? columns_[which] : current_column_;
        // BOOST_ASSERT_MSG(col.GetColumnNumber() == which, std::format("Wrong
        // column number: {}. Was expecting: {}", col.GetColumnNumber(),
        // which).c_str());
        return col;
    }

   protected:
    // ====================  DATA MEMBERS
    // =======================================

   private:
    friend class PF_Chart_Iterator;
    friend class PF_Chart_ReverseIterator;

    [[nodiscard]] std::string MakeChartBaseName() const;

    void FromJSON(const Json::Value &new_data);

    // ====================  DATA MEMBERS
    // =======================================

    Boxes boxes_;
    PF_SignalList signals_;
    std::vector<PF_Column> columns_;
    PF_Column current_column_;

    std::string symbol_;
    std::string chart_base_name_;

    decimal::Decimal base_box_size_ = 0;   // box size to use when constructing file name
    decimal::Decimal fname_box_size_ = 0;  // box size to use when constructing file name
    decimal::Decimal box_size_modifier_ = 0;

    PF_Column::TmPt first_date_ = {};         //	earliest entry for symbol
    PF_Column::TmPt last_change_date_ = {};   //	date of last change to
                                              // data
    PF_Column::TmPt last_checked_date_ = {};  //	last time checked to see if update needed

    decimal::Decimal y_min_ = 100000;  // just a number
    decimal::Decimal y_max_ = -1;

    PF_Column::Direction current_direction_ = PF_Column::Direction::e_Unknown;

    int64_t max_columns_for_graph_ = 0;  // how many columns to show in graphic
};                                       // -----  end of class PF_Chart  -----

// =====================================================================================
//        Class:  PF_Chart_Iterator
//  Description:  std compatable iterator for chart columns
//
// =====================================================================================
class PF_Chart::PF_Chart_Iterator
{
   public:
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = PF_Column;
    using difference_type = std::ptrdiff_t;
    using pointer = const PF_Column *;
    using reference = const PF_Column &;

   public:
    // ====================  LIFECYCLE
    // =======================================

    PF_Chart_Iterator() = default;
    explicit PF_Chart_Iterator(const PF_Chart *chart, int32_t index = 0) : chart_{chart}, index_{index} {}

    // ====================  ACCESSORS
    // =======================================

    // ====================  MUTATORS
    // =======================================

    // ====================  OPERATORS
    // =======================================

    bool operator==(const PF_Chart_Iterator &rhs) const;
    bool operator!=(const PF_Chart_Iterator &rhs) const { return !(*this == rhs); }

    reference operator*() const { return (*chart_)[index_]; }
    pointer operator->() const { return &(*chart_)[index_]; }

    PF_Chart_Iterator &operator++();
    PF_Chart_Iterator operator++(int)
    {
        PF_Chart_Iterator retval = *this;
        ++(*this);
        return retval;
    }
    PF_Chart_Iterator &operator+=(difference_type n);
    PF_Chart_Iterator operator+(difference_type n)
    {
        PF_Chart_Iterator retval = *this;
        retval += n;
        return retval;
    }

    PF_Chart_Iterator &operator--();
    PF_Chart_Iterator operator--(int)
    {
        PF_Chart_Iterator retval = *this;
        --(*this);
        return retval;
    }
    PF_Chart_Iterator &operator-=(difference_type n);
    PF_Chart_Iterator operator-(difference_type n)
    {
        PF_Chart_Iterator retval = *this;
        retval -= n;
        return retval;
    }

    reference operator[](difference_type n) const { return (*chart_)[n]; }

   protected:
    // ====================  METHODS =======================================

    // ====================  DATA MEMBERS
    // =======================================

   private:
    // ====================  METHODS =======================================

    // ====================  DATA MEMBERS
    // =======================================

    const PF_Chart *chart_ = nullptr;
    int32_t index_ = -1;

};  // -----  end of class PF_Chart_Iterator  -----

// =====================================================================================
//        Class:  PF_Chart_ReverseIterator
//  Description:  std compatable iterator for chart columns
//
// =====================================================================================
class PF_Chart::PF_Chart_ReverseIterator
{
   public:
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = PF_Column;
    using difference_type = std::ptrdiff_t;
    using pointer = const PF_Column *;
    using reference = const PF_Column &;

   public:
    // ====================  LIFECYCLE
    // =======================================

    PF_Chart_ReverseIterator() = default;
    explicit PF_Chart_ReverseIterator(const PF_Chart *chart, int32_t index) : chart_{chart}, index_{index} {}

    // ====================  ACCESSORS
    // =======================================

    // ====================  MUTATORS
    // =======================================

    // ====================  OPERATORS
    // =======================================

    bool operator==(const PF_Chart_ReverseIterator &rhs) const;
    bool operator!=(const PF_Chart_ReverseIterator &rhs) const { return !(*this == rhs); }

    reference operator*() const { return (*chart_)[index_]; }
    pointer operator->() const { return &(*chart_)[index_]; }

    PF_Chart_ReverseIterator &operator++();
    PF_Chart_ReverseIterator operator++(int)
    {
        PF_Chart_ReverseIterator retval = *this;
        ++(*this);
        return retval;
    }
    PF_Chart_ReverseIterator &operator+=(difference_type n);
    PF_Chart_ReverseIterator operator+(difference_type n)
    {
        PF_Chart_ReverseIterator retval = *this;
        retval += n;
        return retval;
    }

    PF_Chart_ReverseIterator &operator--();
    PF_Chart_ReverseIterator operator--(int)
    {
        PF_Chart_ReverseIterator retval = *this;
        --(*this);
        return retval;
    }
    PF_Chart_ReverseIterator &operator-=(difference_type n);
    PF_Chart_ReverseIterator operator-(difference_type n)
    {
        PF_Chart_ReverseIterator retval = *this;
        retval -= n;
        return retval;
    }

    reference operator[](difference_type n) const { return (*chart_)[chart_->size() - n - 1]; }

   protected:
    // ====================  METHODS =======================================

    // ====================  DATA MEMBERS
    // =======================================

   private:
    // ====================  METHODS =======================================

    // ====================  DATA MEMBERS
    // =======================================

    const PF_Chart *chart_ = nullptr;
    int32_t index_ = -1;

};  // -----  end of class PF_Chart_ReverseIterator  -----

template <>
struct std::formatter<PF_Chart> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Chart &chart, std::format_context &ctx) const
    {
        namespace rng = std::ranges;
        std::string s;
        std::format_to(std::back_inserter(s),
                       "chart for ticker: {}. box size: {}. reversal boxes: "
                       "{}. scale: {}.\n",
                       chart.GetSymbol(), chart.GetChartBoxSize().format("f"), chart.GetReversalboxes(),
                       chart.GetBoxScale());
        rng::for_each(chart, [&s](const auto &col) { std::format_to(std::back_inserter(s), "\t{}\n", col); });
        std::format_to(std::back_inserter(s), "number of columns: {}. min value: {}. max value: {}.\n", chart.size(),
                       chart.GetYLimits().first.format("f"), chart.GetYLimits().second.format("f"));

        std::format_to(std::back_inserter(s), "{}\n", chart.GetBoxes());

        std::format_to(std::back_inserter(s), "Signals:\n");
        for (const auto &sig : chart.GetSignals())
        {
            std::format_to(std::back_inserter(s), "\t{}\n", sig);
        }

        return formatter<std::string>::format(s, ctx);
    }
};

inline std::ostream &operator<<(std::ostream &os, const PF_Chart &chart)
{
    std::format_to(std::ostream_iterator<char>{os}, "{}\n", chart);

    return os;
}

// for those times you need a name but don't have the chart object yet.

std::string MakeChartNameFromParams(const PF_Chart::PF_ChartParams &vals, std::string_view interval,
                                    std::string_view suffix);

// In order to set our box size, we use the Average True Range method. For
// each symbol, we will look up the 'n' most recent historical values
// starting with the given date and moving backward from
// We expect the process which collects the data will used adjusted values or
// not and populate 'the_data' as required. This simplifies our logic here.

// if we provide a scale factor > -99, then scale the value to that value.
// Otherwise, scale to -3 (.005) to prevent box sizes being too small.
decimal::Decimal ComputeATR(std::string_view symbol, const std::vector<StockDataRecord> &the_data,
                            int32_t how_many_days, int32_t scale = -99);

#endif  // ----- #ifndef PF_CHART_INC  -----
