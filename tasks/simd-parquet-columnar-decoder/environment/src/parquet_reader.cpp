#include "parquet_reader.hpp"
#include "bit_unpack.hpp"
#include <fstream>
#include <iostream>

namespace parquet {

ParquetReader::ParquetReader(const std::string& file_path)
    : file_path_(file_path) {}

ParquetReader::~ParquetReader() {}

bool ParquetReader::Open() {
    std::ifstream file(file_path_, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    file_buffer_.resize(size);
    if (!file.read(reinterpret_cast<char*>(file_buffer_.data()), size)) {
        return false;
    }

    // Populate mock metadata for starter testbed
    metadata_.version = 1;
    metadata_.num_rows = 1000;
    metadata_.column_names = {"id", "value"};
    metadata_.column_types = {Type::INT32, Type::INT64};

    RowGroupMeta rg;
    rg.num_rows = 1000;
    rg.total_byte_size = size;

    ColumnChunkMeta col1, col2;
    col1.type = Type::INT32;
    col1.num_values = 1000;
    col1.min_int64 = 0;
    col1.max_int64 = 999;

    col2.type = Type::INT64;
    col2.num_values = 1000;
    col2.min_int64 = 100;
    col2.max_int64 = 10999;

    rg.columns = {col1, col2};
    metadata_.row_groups = {rg};

    return true;
}

std::vector<int32_t> ParquetReader::ReadInt32Column(size_t col_idx, size_t row_group_idx) {
    if (row_group_idx >= metadata_.row_groups.size()) return {};
    std::vector<int32_t> res(metadata_.row_groups[row_group_idx].num_rows);
    for (size_t i = 0; i < res.size(); ++i) {
        res[i] = static_cast<int32_t>(i);
    }
    return res;
}

std::vector<int64_t> ParquetReader::ReadInt64Column(size_t col_idx, size_t row_group_idx) {
    if (row_group_idx >= metadata_.row_groups.size()) return {};
    std::vector<int64_t> res(metadata_.row_groups[row_group_idx].num_rows);
    for (size_t i = 0; i < res.size(); ++i) {
        res[i] = static_cast<int64_t>(i * 10);
    }
    return res;
}

std::vector<uint8_t> ParquetReader::EvaluateFilter(const FilterPredicate& pred, size_t row_group_idx) {
    if (row_group_idx >= metadata_.row_groups.size()) return {};
    const auto& rg = metadata_.row_groups[row_group_idx];
    if (pred.column_idx < rg.columns.size()) {
        const auto& col = rg.columns[pred.column_idx];
        if (col.min_int64 && col.max_int64) {
            // Prune entire row group if disjoint
            if (*col.max_int64 < pred.gt_val || *col.min_int64 > pred.lt_val) {
                return std::vector<uint8_t>(rg.num_rows, 0);
            }
        }
    }
    return std::vector<uint8_t>(rg.num_rows, 1);
}

} // namespace parquet
