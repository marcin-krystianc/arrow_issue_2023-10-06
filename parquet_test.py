import numpy as np
import pyarrow as pa
import pyarrow.parquet as pq

import time
import polars as pl
import csv
import gc

t_write = []
t_read_100_pre_buffer = []

path = "/tmp/test_wide.parquet"

columns_list = [
                100, 200, 300, 400, 500, 600, 700, 800, 900,              
                1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
                10_000, 20_000, 30_000, 40_000, 50_000, 
                ]

chunks_list = [1000, 10_000]
rows_lsit = [5000]
with open('results_python.csv', 'w', encoding='UTF8', newline='') as f:

    writer = csv.writer(f)
    # write the header
    writer.writerow(['columns','rows','chunk_size','writing(μs)','reading_all(μs)','reading_100(μs)'])

    for chunk_size in chunks_list:
        for rows in rows_lsit:
            for columns in columns_list:
      
                table = pl.DataFrame(
                    data=np.random.randn(rows, columns),
                    schema=[f"c{i}" for i in range(columns)]).to_arrow()

                t = time.time()
                pq.write_table(table, path, row_group_size=chunk_size, use_dictionary=False, write_statistics=False)
                t_writing = time.time() - t
                t_write.append(t_writing)

                del table
                gc.collect()

                t_read = []
                t_read_100 = []

                for i in range(0, 3):

                    t = time.time()
                    res = pq.read_table(path, use_threads=False)
                    t_read.append(time.time() - t)
        
                    del res 
                    gc.collect()

                    t = time.time()               
                    res_100 = pq.read_table(path, columns=[f"c{i}" for i in range(100)], use_threads=False)
                    t_read_100.append(time.time() - t)    
                
                    del res_100
                    gc.collect()

                t_reading = min(t_read)
                t_reading_100 = min(t_read_100)

                data = [columns, rows, chunk_size, t_writing * 1_000_000, t_reading * 1_000_000, t_reading_100 * 1_000_000]
                writer.writerow(data)
                print(str(data))
