#pragma once
#include "parquet_types.hpp"
#include <string>
#include <vector>
#include <memory>

namespace parquet {

struct FilterPredicate {
    size_t column_idx{0};
    int64_t gt_val{INT64_MIN};
    int64_t lt_val{INT64_MAX};
};

class ParquetReader {
public:
    explicit ParquetReader(const std::string& file_path);
    ~ParquetReader();

    bool Open();
    const FileMetaData& GetMetadata() const { return metadata_; }

    std::vector<int32_t> ReadInt32Column(size_t col_idx, size_t row_group_idx);
    std::vector<int64_t> ReadInt64Column(size_t col_idx, size_t row_group_idx);
    std::vector<uint8_t> EvaluateFilter(const FilterPredicate& pred, size_t row_group_idx);

private:
    std::string file_path_;
    FileMetaData metadata_;
    std::vector<uint8_t> file_buffer_;
};

} // namespace parquet
