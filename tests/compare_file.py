import sys
import ntpath
import subprocess

"""
READ ARGUMENTS
"""

if len(sys.argv) < 4:
    print("Usage: python compare_file.py seq_list_file seq_dir output_path")
    print("where seq_list_file is the file with the list of sequences that we pass to hrlz,")
    print("seq_dir is the directory where the sequences are, and")
    print("output_path is the directory where the output sequences are")
    sys.exit(1)

seq_file = sys.argv[1]
sequences_path = sys.argv[2]
output_path = sys.argv[3]

if sequences_path[-1] != "/":
    sequences_path = sequences_path + "/"
if output_path[-1] != "/":
    output_path = output_path + "/"

""" OLD:
#Directory of the data
data_path = "/home/max/Documents/DTU/PHD COMPRESSION/Programs/DATA"
#File with the sequences that we pass to hrlz to compress
seq_file = data_path + "/absolute_sequences.txt"
#Original directory of the sequences
sequences_path = data_path + "/"
#Output of the decompression
output_path = data_path + "/output_decomp/"

"""

sequence_names = []
count = -1

#Read the file with the filenames
#We will gather a list of the filenames that were used for compression
with open(seq_file) as filenames:
    i = 0
    for line in filenames:
        if i == 0:
            count = int(line.strip())
            i+=1
        elif i <= count:
            name = ntpath.basename(line.strip())
            sequence_names.append(name)
            i+=1
        else:
            break
print("Comparing:")
print(sequence_names)

#Compare
for i in range(count):
    #print("Running: ", "compare_fasta", str(sequences_path + sequence_names[i]), str(output_path + sequence_names[i]))
    subprocess.run(["../build/tests/compare_fasta", str(sequences_path + sequence_names[i]), str(output_path + sequence_names[i])], check=True)
