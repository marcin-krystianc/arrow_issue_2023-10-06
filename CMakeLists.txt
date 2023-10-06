cmake_minimum_required(VERSION 3.16)

project(ColumnReadingPerf)

option(ARROW_LINK_SHARED "Link to the Arrow shared library" ON)

find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)


message(STATUS "Arrow version: ${ARROW_VERSION}")
message(STATUS "Arrow SO version: ${ARROW_FULL_SO_VERSION}")
message(STATUS "Parquet version: ${PARQUET_VERSION}")
message(STATUS "Parquet SO version: ${PARQUET_FULL_SO_VERSION}")

add_executable(ColumnReadingPerf ColumnReadingPerf.cc)
target_link_libraries(ColumnReadingPerf PRIVATE Arrow::arrow_shared Parquet::parquet_shared)