// =====================================================================================
// 
//       Filename:  p_f_data.h
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
#include <vector>

#include <date/date.h>

#include "utilities.h"

namespace fs = std::filesystem;

//using namespace std::chrono_literals;

#include "PF_Column.h"


class PF_Chart
{
public:

    using Y_Limits = std::pair<DprDecimal::DDecDouble, DprDecimal::DDecDouble>;

    // make it look like a range

    using const_iterator = std::vector<PF_Column>::const_iterator;

    // ====================  LIFECYCLE     =======================================
    PF_Chart () = default;                             // constructor
    PF_Chart(const std::string& symbol, DprDecimal::DDecDouble boxsize, int32_t reversal_boxes,
            PF_Column::FractionalBoxes fractional_boxes=PF_Column::FractionalBoxes::e_integral);

    // ====================  ACCESSORS     =======================================

    void ExportData(std::ostream* output_data);
    [[nodiscard]] PF_Column::Direction GetCurrentDirection() const { return current_direction_; }

    [[nodiscard]] std::size_t GetNumberOfColumns() const { return columns_.size(); }

    [[nodiscard]] Y_Limits GetYLimits() const { return {y_min_, y_max_}; }

    [[nodiscard]] const_iterator begin() const { return columns_.begin(); }
    [[nodiscard]] const_iterator end() const { return columns_.end(); }

    void ConstructChartAndWriteToFile(fs::path output_filename) const;

    [[nodiscard]] Json::Value ToJSON() const;

    // ====================  MUTATORS      =======================================
    
    template<typename T>
    void LoadData(std::istream* input_data, std::string_view format, char delim);

    // ====================  OPERATORS     =======================================

    const PF_Column& operator[](size_t which) const { return columns_[which]; }
	friend std::ostream& operator<<(std::ostream& os, const PF_Chart& chart);

protected:
    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  DATA MEMBERS  =======================================

    std::vector<PF_Column> columns_;
    std::unique_ptr<PF_Column> current_column_;

    std::string symbol_;

    PF_Column::tpt mFirstDate_;			    //	earliest entry for symbol
    PF_Column::tpt mLastChangeDate_;		//	date of last change to data
    PF_Column::tpt mLastCheckedDate_;	    //	last time checked to see if update needed

    DprDecimal::DDecDouble box_size_;
    int32_t reversal_boxes_;
    DprDecimal::DDecDouble y_min_ = 100000;         // just a number
    DprDecimal::DDecDouble y_max_ = -1;

    PF_Column::Direction current_direction_;
    PF_Column::FractionalBoxes fractional_boxes_;

}; // -----  end of class PF_Chart  -----

template<typename T>
void PF_Chart::LoadData (std::istream* input_data, std::string_view format, char delim)
{
    // for now, just assume its numbers.

    current_column_ = std::make_unique<PF_Column>(box_size_, reversal_boxes_, fractional_boxes_);

    std::string buffer;
    while ( ! input_data->eof())
    {
        std::getline(*input_data, buffer);
        if (input_data->fail())
        {
            continue;
        }
        // need to split out fields

        auto fields = split_string<std::string_view>(buffer, delim);

        PF_Column::tpt the_time = StringToTimePoint("%Y-%m-%d", fields[0]);
        auto [status, new_col] = current_column_->AddValue(DprDecimal::DDecDouble(fields[1]), the_time);

        if (current_column_->GetTop() > y_max_)
        {
            y_max_ = current_column_->GetTop();
        }
        if (current_column_->GetBottom() < y_min_)
        {
            y_min_ = current_column_->GetBottom();
        }

        if (status == PF_Column::Status::e_reversal)
        {
            auto* save_col = current_column_.get();         // non-owning access
            columns_.push_back(*save_col);
            current_column_ = std::move(new_col.value());

            // now continue on processing the value.
            
            status = current_column_->AddValue(DprDecimal::DDecDouble(fields[1]), the_time).first;
            current_direction_ = current_column_->GetDirection();
        }
    }

    // make sure we keep the last column we were working on 

    if (current_column_->GetTop() > y_max_)
    {
        y_max_ = current_column_->GetTop();
    }
    if (current_column_->GetBottom() < y_min_)
    {
        y_min_ = current_column_->GetBottom();
    }

    columns_.push_back(*current_column_);

    PF_Column* col = current_column_.release();
    delete col;
}

inline std::ostream& operator<<(std::ostream& os, const PF_Chart& chart)
{
    os << "chart for ticker: " << chart.symbol_ << " box size: " << chart.box_size_ << " reversal boxes: " << chart.reversal_boxes_<< '\n';
    for (const auto& col : chart.columns_)
    {
        os << '\t' << col << '\n';
    }
    os << "number of columns: " << chart.columns_.size() << " min value: " << chart.y_min_ << " max value: " << chart.y_max_ << '\n';
    return os;
}

#endif   // ----- #ifndef PF_CHART_INC  ----- 

