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

#ifndef  PF_CHART_INC
#define  PF_CHART_INC


#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <date/date.h>

#include <pqxx/pqxx>
#include <pqxx/transaction.hxx>

//namespace fs = std::filesystem;

//using namespace std::chrono_literals;

#include "Boxes.h"
#include "PF_Column.h"
#include "PointAndFigureDB.h"
#include "utilities.h"

class PF_Chart
{
public:

    using Y_Limits = std::pair<DprDecimal::DDecQuad, DprDecimal::DDecQuad>;
    using PF_ChartParams = std::tuple<std::string, DprDecimal::DDecQuad, int32_t, Boxes::BoxScale>;

    enum  class X_AxisFormat {e_show_date, e_show_time};
	enum {
		e_symbol = 0,
		e_box_size,
		e_reversal,
		e_box_scale
	};
    // make it look like a range
    // TODO(dpriedel): use a custom iterator which will include current_column_ in 
    // the iteration.

//    using const_iterator = std::vector<PF_Column>::const_iterator;

    // ====================  LIFECYCLE     =======================================
    PF_Chart () = default;                             // constructor
    PF_Chart (const PF_Chart& rhs);
    PF_Chart (PF_Chart&& rhs) noexcept ;

    PF_Chart(std::string symbol, DprDecimal::DDecQuad box_size, int32_t reversal_boxes,
            Boxes::BoxScale box_scale=Boxes::BoxScale::e_linear,
            DprDecimal::DDecQuad atr=0, int64_t max_columns_for_graph=0);

    PF_Chart(const PF_ChartParams& vals, DprDecimal::DDecQuad atr, int64_t max_columns_for_graph)
        : PF_Chart(std::get<e_symbol>(vals), std::get<e_box_size>(vals), std::get<e_reversal>(vals),
        		std::get<e_box_scale>(vals), atr, max_columns_for_graph)
    {
    }

    explicit PF_Chart(const Json::Value& new_data);

    ~PF_Chart() = default;

    static PF_Chart MakeChartFromDB(const PF_DB& chart_db, PF_ChartParams vals);

    // ====================  ACCESSORS     =======================================

	[[nodiscard]] bool IsEmpty() const { return columns_.empty() && current_column_.IsEmpty(); }
    [[nodiscard]] DprDecimal::DDecQuad GetChartBoxSize() const { return boxes_.GetBoxSize(); }
    [[nodiscard]] DprDecimal::DDecQuad GetFNameBoxSize() const { return fname_box_size_; }
    [[nodiscard]] int32_t GetReversalboxes() const { return current_column_.GetReversalboxes(); }
    [[nodiscard]] Boxes::BoxScale GetBoxScale() const { return boxes_.GetBoxScale(); }
    [[nodiscard]] Boxes::BoxType GetBoxType() const { return boxes_.GetBoxType(); }
    [[nodiscard]] std::string GetSymbol() const { return symbol_; }

    [[nodiscard]] PF_Column::Direction GetCurrentDirection() const { return current_direction_; }

    [[nodiscard]] std::string ChartName(std::string_view suffix) const;

    // for those times you need a name but don't have the chart object yet.

    static std::string ChartName(const PF_ChartParams& vals, std::string_view suffix);

    // includes 'current_column'
    [[nodiscard]] std::size_t GetNumberOfColumns() const { return columns_.size() + 1; }

    [[nodiscard]] Y_Limits GetYLimits() const { return {y_min_, y_max_}; }

    [[nodiscard]] PF_Column::TmPt GetFirstTime() const { return first_date_; }
    [[nodiscard]] PF_Column::TmPt GetLastChangeTime() const { return last_change_date_; }
    [[nodiscard]] PF_Column::TmPt GetLastCheckedTime() const { return last_checked_date_; }

    void ConstructChartGraphAndWriteToFile(const fs::path& output_filename, const std::string& show_trend_lines, X_AxisFormat date_or_time=X_AxisFormat::e_show_date) const;

    void ConvertChartToJsonAndWriteToFile(const fs::path& output_filename) const;
    void ConvertChartToJsonAndWriteToStream(std::ostream& stream) const;

    void ConvertChartToTableAndWriteToFile(const fs::path& output_filename, X_AxisFormat date_or_time=X_AxisFormat::e_show_date) const;
    void ConvertChartToTableAndWriteToStream(std::ostream& stream, X_AxisFormat date_or_time=X_AxisFormat::e_show_date) const;

    void StoreChartInChartsDB(const PF_DB& chart_db, X_AxisFormat date_or_time=X_AxisFormat::e_show_date, bool store_cvs_graphics=false);

    [[nodiscard]] Json::Value ToJSON() const;
    [[nodiscard]] bool IsPercent() const { return boxes_.GetBoxScale() == Boxes::BoxScale::e_percent; }
    [[nodiscard]] bool IsFractional() const { return boxes_.GetBoxType() == Boxes::BoxType::e_fractional; }

    [[nodiscard]] const Boxes& GetBoxes() const { return boxes_; }
    [[nodiscard]] const std::vector<PF_Column>& GetColumns() const { return columns_; }
    [[nodiscard]] const PF_Column& GetCurrentColumn() const { return current_column_; }

    [[nodiscard]] PF_ChartParams GetChartParams() const { return {symbol_, fname_box_size_, current_column_.GetReversalboxes(), boxes_.GetBoxScale()}; }

    // ====================  MUTATORS      =======================================
    
    PF_Column::Status AddValue(const DprDecimal::DDecQuad& new_value, PF_Column::TmPt the_time);
    void LoadData(std::istream* input_data, std::string_view date_format, char delim);

    void SetMaxGraphicColumns(int64_t max_cols) { max_columns_for_graph_ = max_cols; }

    // ====================  OPERATORS     =======================================

    PF_Chart& operator= (const PF_Chart& rhs);
    PF_Chart& operator= (PF_Chart&& rhs) noexcept ;

    PF_Chart& operator= (const Json::Value& new_data);

    bool operator== (const PF_Chart& rhs) const;
    bool operator!= (const PF_Chart& rhs) const { return ! operator==(rhs); }

    const PF_Column& operator[](size_t which) const { return (which < columns_.size() ? columns_[which] : current_column_); }

protected:
    // ====================  DATA MEMBERS  =======================================

private:

    void FromJSON(const Json::Value& new_data);

    // ====================  DATA MEMBERS  =======================================

    Boxes boxes_;
    std::vector<PF_Column> columns_;
    PF_Column current_column_;

    std::string symbol_;
	DprDecimal::DDecQuad fname_box_size_ = 0;	// box size to use when constructing file name 
	DprDecimal::DDecQuad atr_ = 0;

    PF_Column::TmPt first_date_ = {};			    //	earliest entry for symbol
    PF_Column::TmPt last_change_date_ = {};		//	date of last change to data
    PF_Column::TmPt last_checked_date_ = {};	    //	last time checked to see if update needed

    DprDecimal::DDecQuad y_min_ = 100000;         // just a number
    DprDecimal::DDecQuad y_max_ = -1;

    PF_Column::Direction current_direction_ = PF_Column::Direction::e_unknown;

	int64_t max_columns_for_graph_ = 0;			// how many columns to show in graphic
}; // -----  end of class PF_Chart  -----

template <> struct fmt::formatter<PF_Chart>: formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Chart& chart, fmt::format_context& ctx)
    {
        std::string s;
        fmt::format_to(std::back_inserter(s), "chart for ticker: {} box size: {} reversal boxes: {} scale: {}\n",
            chart.GetSymbol(), chart.GetChartBoxSize(), chart.GetReversalboxes(), chart.GetBoxScale());
    	for (const auto& col : chart.GetColumns())
    	{
        	fmt::format_to(std::back_inserter(s), "\t{}\n", col);
    	}
    	fmt::format_to(std::back_inserter(s), "\tcurrent column: {}\n", chart.GetCurrentColumn());
    	fmt::format_to(std::back_inserter(s), "number of columns: {} min value: {} max value: {}\n",
        	chart.GetNumberOfColumns(), chart.GetYLimits().first, chart.GetYLimits().second);

        fmt::format_to(std::back_inserter(s), "{}\n", chart.GetBoxes());
        return formatter<std::string>::format(s, ctx);
    }
};

inline std::ostream& operator<<(std::ostream& os, const PF_Chart& chart)
{
    fmt::format_to(std::ostream_iterator<char>{os}, "{}\n", chart);

    return os;
}

// In order to set our box size, we use the Average True Range method. For
// each symbol, we will look up the 'n' most recent historical values 
// starting with the given date and moving backward from 
// Tiingo and compute the ATR using the formula from Investopedia.

DprDecimal::DDecQuad ComputeATRUsingJSON(std::string_view symbol, const Json::Value& the_data, int32_t how_many_days, UseAdjusted use_adjusted=UseAdjusted::e_No);

// we need to be able to use data stored in our stock prices DB too

DprDecimal::DDecQuad ComputeATRUsingDB(std::string_view symbol, const pqxx::result& results, int32_t how_many_days);

#endif   // ----- #ifndef PF_CHART_INC  ----- 

