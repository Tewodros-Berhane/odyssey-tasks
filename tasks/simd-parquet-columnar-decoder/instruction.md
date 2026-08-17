# Zero-Copy SIMD Columnar Parquet Decoder with Predicate Pushdown

## Overview
Your objective is to implement a high-throughput, zero-copy Apache Parquet / Arrow columnar format decoder in C++20 with AVX2 SIMD acceleration and predicate pushdown in `/app`.

## Architecture & Requirements

### 1. Parquet Metadata & Thrift Footer Parsing
- Parse the 4-byte magic bytes `PAR1` at header and footer.
- Parse FileMetaData Thrift structure at the end of the file:
  - Schema elements (Field names, primitive types `INT32`, `INT64`, `FLOAT`, `DOUBLE`, `BYTE_ARRAY`, `FIXED_LEN_BYTE_ARRAY`, logical types).
  - RowGroups metadata, ColumnChunks, and PageHeaders.
  - ColumnChunk statistics: `min_value`, `max_value`, `null_count`, `distinct_count`.

### 2. SIMD Accelerated Bit-Unpacking & RLE Decoding
- Implement hybrid Run-Length Encoding (RLE) and Bit-Packing (RFC / Parquet spec):
  - RLE run: Repeated literal runs with count header.
  - Bit-packed run: 8-group batches of bit-packed integers.
  - Accelerated AVX2 bit-unpacking: Unpack 32 values in parallel using `_mm256_loadu_si256`, `_mm256_shuffle_epi8`, `_mm256_srlv_epi32`, `_mm256_and_si256`.
- Support bit-widths from 1 to 32 bits per value.

### 3. Page Encodings & Compression
- Encodings:
  - `PLAIN`: Direct little-endian binary values.
  - `PLAIN_DICTIONARY` / `RLE_DICTIONARY`: Decode dictionary page, followed by dictionary index stream.
  - `RLE`: Definition and repetition levels.
- Compression codecs: `UNCOMPRESSED`, `SNAPPY`, `ZSTD`.

### 4. Vectorized Predicate Pushdown
- Support predicate filters (e.g. `col > val`, `col <= val`, `col == val`, `col BETWEEN a AND b`):
  - Prune entire RowGroups using min/max statistics without reading page data.
  - Evaluate filters directly on dictionary indices before dictionary lookup.
  - Output bitmask vectors indicating matching row indices.

## Build and Test Instructions
The project uses CMake and C++20:
```bash
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
ctest --output-on-failure
```

The verifier executes `tests/test.sh`, which evaluates Thrift header decoding, SIMD bit-unpacking correctness, dictionary decoding, and predicate pushdown scan throughput (>= 2.0 GB/s).
