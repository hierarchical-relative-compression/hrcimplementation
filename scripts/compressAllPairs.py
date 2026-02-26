from pathlib import Path
from compressionTask import CompressionTask
from multiprocessing import Pool
from collections import defaultdict
import shutil


SEQ_DIR = Path("./sequences")
TMP_FILES = Path("./tmp")
RES_FILES = Path("./res")
TMP_FILES.mkdir(exist_ok=True)
RES_FILES.mkdir(exist_ok=True)

adj_file = open("adj", "w")

p = Pool(12)

sequences = sorted(list(filter(lambda s: s.suffix != ".sa", SEQ_DIR.iterdir())))
tasks = []
for ref in sequences:
    others = [v for v in sequences]
    others.remove(ref)
    tasks.append(CompressionTask(ref, others, Path(TMP_FILES, ref.name), Path(RES_FILES, f"{ref.name}.stdout"), Path(RES_FILES, f"{ref.name}.stderr")))

print(f"Processing {len(sequences)} sequences")
tasks = p.map(CompressionTask.execute, tasks)


adj_file.write(f"{len(sequences) * (len(sequences) - 1)}\n")
for (task, result) in tasks:
    for seq, fac in zip(task.files, result):
        adj_file.write(f"{task.reference} {seq} {fac}\n")

shutil.rmtree(TMP_FILES)
adj_file.close()

adj_file.close()
