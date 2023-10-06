#include "arrow/api.h"
#include "arrow/io/api.h"
#include "arrow/result.h"
#include "arrow/util/type_fwd.h"
#include "parquet/arrow/reader.h"
#include "parquet/arrow/writer.h"

#include <iostream>
#include <list>
#include <chrono>
#include <random>
#include <vector>
#include <fstream>
#include <iomanip>

using arrow::Status;

namespace
{
  const char *FILE_NAME = "/tmp/my_cpp.parquet";

  std::shared_ptr<arrow::Table> GetTable(size_t nColumns, size_t nRows)
  {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> rand_gen(0.0, 1.0);

    std::vector<std::shared_ptr<arrow::Array>> arrays;
    std::vector<std::shared_ptr<arrow::Field>> fields;

    // For simplicity, we'll create int32 columns. You can expand this to handle other types.
    for (int i = 0; i < nColumns; i++)
    {
      arrow::DoubleBuilder builder;
      for (auto j = 0; j < nRows; j++)
      {
        if (!builder.Append(rand_gen(rng)).ok())
          throw std::runtime_error("builder.Append");
      }

      std::shared_ptr<arrow::Array> array;
      if (!builder.Finish(&array).ok())
        throw std::runtime_error("builder.Finish");

      arrays.push_back(array);
      fields.push_back(arrow::field("c_" + std::to_string(i), arrow::float64(), false));
    }

    auto table = arrow::Table::Make(arrow::schema(fields), arrays);
    return table;
  }

  Status WriteTableToParquet(size_t nColumns, size_t nRows, const std::string &filename, std::chrono::microseconds *dt, int64_t chunkSize)
  {
    auto table = GetTable(nColumns, nRows);
    auto begin = std::chrono::steady_clock::now();
    auto result = arrow::io::FileOutputStream::Open(filename);
    auto outfile = result.ValueOrDie();
    parquet::WriterProperties::Builder builder;
    auto properties = builder
                          .max_row_group_length(chunkSize)
                          ->disable_dictionary()
                          ->build();
    PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, chunkSize, properties));
    auto end = std::chrono::steady_clock::now();
    *dt = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    return Status::OK();
  }

  Status ReadEntireTable(const std::string &filename, std::chrono::microseconds *dt)
  {
    auto begin = std::chrono::steady_clock::now();

    std::shared_ptr<arrow::io::ReadableFile> infile;
    ARROW_ASSIGN_OR_RAISE(infile, arrow::io::ReadableFile::Open(filename));
    std::unique_ptr<parquet::arrow::FileReader> reader;
    ARROW_RETURN_NOT_OK(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));
    std::shared_ptr<arrow::Table> parquet_table;

    // Read the table.
    PARQUET_THROW_NOT_OK(reader->ReadTable(&parquet_table));
    auto end = std::chrono::steady_clock::now();
    *dt = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    return Status::OK();
  }

  Status ReadColumnsAsTable(const std::string &filename, std::vector<int> indicies, std::chrono::microseconds *dt)
  {
    auto begin = std::chrono::steady_clock::now();

    std::shared_ptr<arrow::io::ReadableFile> infile;
    ARROW_ASSIGN_OR_RAISE(infile, arrow::io::ReadableFile::Open(filename));
    std::unique_ptr<parquet::arrow::FileReader> reader;
    ARROW_RETURN_NOT_OK(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));

    // Read the table.
    std::shared_ptr<arrow::Table> parquet_table;
    PARQUET_THROW_NOT_OK(reader->ReadTable(indicies, &parquet_table));
    auto end = std::chrono::steady_clock::now();
    *dt = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    return Status::OK();
  }

  Status RunMain(int argc, char **argv)
  {
    std::ofstream csvFile;
    csvFile.open("results_cpp.csv", std::ios_base::out); // append instead of overwrite
    csvFile << "columns, rows, chunk_size, writing(μs), reading_all(μs), reading_100(μs)" << std::endl;

    std::list<int> nColumns = {
        100, 200, 300, 400, 500, 600, 700, 800, 900,
        1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
        10000, 20000, 30000, 40000, 50000};

    std::list<int64_t> chunk_sizes = {1000, 100000};
    std::list<int> rows_list = {5000};

    std::vector<int> indicies(100);
    std::iota(indicies.begin(), indicies.end(), 0);

    for (auto chunk_size : chunk_sizes)
    {
      for (int nRow : rows_list)
      {
        for (int nColumn : nColumns)
        {
          std::chrono::microseconds writing_dt;
          ARROW_RETURN_NOT_OK(WriteTableToParquet(nColumn, nRow, FILE_NAME, &writing_dt, chunk_size));

          const int repeats = 3;
          std::vector<std::chrono::microseconds> reading_all_dts(repeats);
          std::vector<std::chrono::microseconds> reading_100_dts(repeats);
          for (int i = 0; i < repeats; i++)
          {
            ARROW_RETURN_NOT_OK(ReadEntireTable(FILE_NAME, &reading_all_dts[i]));
            ARROW_RETURN_NOT_OK(ReadColumnsAsTable(FILE_NAME, indicies, &reading_100_dts[i]));
          }

          auto reading_all_dt = *std::min_element(reading_all_dts.begin(), reading_all_dts.end());
          auto reading_100_dt = *std::min_element(reading_100_dts.begin(), reading_100_dts.end());

          std::cerr << "(" << nColumn << ", " << nRow << ")"
                    << ", chunk_size=" << chunk_size
                    << ", writing_dt=" << writing_dt.count() / nColumn
                    << ", reading_all_dt=" << reading_all_dt.count() / nColumn
                    << ", reading_100_dt=" << reading_100_dt.count() / 100
                    << std::endl;

          csvFile << nColumn << ","
                  << nRow << ","
                  << chunk_size << ","
                  << writing_dt.count() << ","
                  << reading_all_dt.count() << ","
                  << reading_100_dt.count()
                  << std::endl;
        }
      }
    }

    return Status::OK();
  }
}

int main(int argc, char **argv)
{
  Status st = RunMain(argc, argv);
  if (!st.ok())
  {
    std::cerr << st << std::endl;
    return 1;
  }
  return 0;
}