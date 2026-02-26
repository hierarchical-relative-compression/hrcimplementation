import sys
import os.path
import subprocess

if len(sys.argv) < 5:
    print("Usage: python compare_file_chromosomes.py seq_list_file chr_names original_dir decomp_dir")
    sys.exit(1)

seq_path = sys.argv[1]
chr_names_path = sys.argv[2]
original_dir = sys.argv[3]
decomp_dir = sys.argv[4]

#Open and read filenames
sequence_dirs = []
count = -1
with open(seq_path) as filenames:
    i = 0
    for line in filenames:
        if i == 0:
            count = int(line.strip())
            i+=1
        elif i <= count:
            name = os.path.basename(line.strip())
            sequence_dirs.append(name)
            i+=1
        else:
            break

#Open and read chr_names
chr_names = []
with open(chr_names_path) as chr_names_file:
    for line in chr_names_file.readlines():
        line = line.strip()
        if not line:
            continue
        chr_names.append(line)

#Compare all
for i in range(count):
    for j in range(len(chr_names)):
        print("Running: compare_fasta ", str(os.path.join(original_dir, sequence_dirs[i], chr_names[j])), str(os.path.join(decomp_dir, sequence_dirs[i], chr_names[j])))
        subprocess.run(["../build/tests/compare_fasta", str(os.path.join(original_dir, sequence_dirs[i], chr_names[j])), str(os.path.join(decomp_dir, sequence_dirs[i], chr_names[j]))])
