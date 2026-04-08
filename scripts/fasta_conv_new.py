import sys
import os

DIR = "fasta_sequences"

def indicatorLineToFileName(line):
    return line[1:].split()[0]

import os
if not os.path.exists(DIR):
    os.mkdir(DIR)

filename = None

file = None

seqCount = 0

for line in sys.stdin:
    line = line.strip()
    if len(line) == 0:
        continue
    if line[0] == ">":
        seqCount += 1
        print(f"new sequence {line}");
        filename = indicatorLineToFileName(line)
        file = open(os.path.join(DIR, filename), "w")
        #Add this line with a newline
        line += "\n"
        file.write(line)
    else:
        file.write(line)


