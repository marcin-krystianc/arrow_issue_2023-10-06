import pandas as pd
import matplotlib.pyplot as plt
import sys

plt.rcParams["figure.figsize"] = [7.50, 3.50]
plt.rcParams["figure.autolayout"] = True

path = sys.argv[1]
df = pd.read_csv(path, skipinitialspace=True)
print (df)

df['writing(μs)'] = df['writing(μs)'] / df['columns']
df['reading_all(μs)'] = df['reading_all(μs)'] / df['columns']
df['reading_100(μs)'] = df['reading_100(μs)'] / 100

groups =  df.groupby(['chunk_size', 'rows'])

i = 0
fig, ax = plt.subplots(len(groups), sharex=True)

for ((chunk_size, rows), g) in groups:
    title = "chunk_size={}, rows={}".format(chunk_size, rows)
    print(title)

    ax[i].set_title(title)
    ax[i].plot(g['columns'], g['reading_all(μs)'], label='reading_all(μs) (per column)', marker='o')
    ax[i].plot(g['columns'], g['reading_100(μs)'], label='reading_100(μs) (per column)', marker='o')

    i+=1

fig.suptitle(path)
plt.xlabel('Number of columns in a file')
plt.ylabel('μs per column')
plt.legend()
plt.show()