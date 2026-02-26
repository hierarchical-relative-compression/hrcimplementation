from pathlib import Path
from compressionTask import CompressionTask
from multiprocessing import Pool
from collections import defaultdict
import shutil

TMP_FILES = Path("./tmp")
RES_FILES = Path("./res")
TMP_FILES.mkdir(exist_ok=True)
RES_FILES.mkdir(exist_ok=True)

edge_count = int(input())
edges = [map(Path, input().split(" ")) for _ in range(edge_count)]

adj_list = defaultdict(lambda: [])
for ref, seq in edges:
    adj_list[ref].append(seq)


adj_file = open("adj", "w")
p = Pool(12)

tasks = []
for ref, adjacents in adj_list.items():
    tasks.append(CompressionTask(ref, adjacents, Path(TMP_FILES, ref.name), Path(RES_FILES, f"{ref.name}.stdout"), Path(RES_FILES, f"{ref.name}.stderr")))
tasks = p.map(CompressionTask.execute, tasks)

adj_file.write(f"{edge_count}\n")
for (task, result) in tasks:
    for seq, fac in zip(task.files, result):
        adj_file.write(f"{task.reference} {seq} {fac}\n")

shutil.rmtree(TMP_FILES)
adj_file.close()
