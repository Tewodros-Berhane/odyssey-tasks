#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <span>

namespace parquet {

enum class Type : uint8_t {
    BOOLEAN = 0,
    INT32 = 1,
    INT64 = 2,
    INT96 = 3,
    FLOAT = 4,
    DOUBLE = 5,
    BYTE_ARRAY = 6,
    FIXED_LEN_BYTE_ARRAY = 7
};

enum class Encoding : uint8_t {
    PLAIN = 0,
    PLAIN_DICTIONARY = 2,
    RLE = 3,
    BIT_PACKED = 4,
    DELTA_BINARY_PACKED = 5,
    DELTA_LENGTH_BYTE_ARRAY = 6,
    DELTA_BYTE_ARRAY = 7,
    RLE_DICTIONARY = 8
};

enum class Compression : uint8_t {
    UNCOMPRESSED = 0,
    SNAPPY = 1,
    GZIP = 2,
    LZO = 3,
    BROTLI = 4,
    LZ4 = 5,
    ZSTD = 6
};

struct ColumnChunkMeta {
    Type type{Type::INT32};
    std::vector<Encoding> encodings;
    Compression compression{Compression::UNCOMPRESSED};
    int64_t num_values{0};
    int64_t total_uncompressed_size{0};
    int64_t total_compressed_size{0};
    int64_t data_page_offset{0};
    int64_t dictionary_page_offset{0};
    std::optional<int64_t> min_int64;
    std::optional<int64_t> max_int64;
};

struct RowGroupMeta {
    std::vector<ColumnChunkMeta> columns;
    int64_t total_byte_size{0};
    int64_t num_rows{0};
};

struct FileMetaData {
    int32_t version{1};
    int64_t num_rows{0};
    std::vector<std::string> column_names;
    std::vector<Type> column_types;
    std::vector<RowGroupMeta> row_groups;
};

} // namespace parquet
