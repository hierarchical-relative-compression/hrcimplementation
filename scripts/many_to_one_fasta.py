import sys
import os
import pathlib

"""
This program takes as input an uncompressed FASTA or FASTQ file, that contains
one or more named FASTA sequences inside. It extracts the FASTA definition line of
all the subparts, and puts it into a separate metadata file along with the position of
the file where it appears. Then, it compacts all the genomic data into a single line.
The output is two files: One with the first FASTA definition and all the genomic
data put into one line, and another with the metadata of all the fasta definition lines
(including the first one).

The input parameter is the file name, optionally with a file path.
"""

if len(sys.argv) < 2:
    print("Usage: many_to_one_fasta.py /path/to/file.fa")
    sys.exit(1)

p = pathlib.Path(sys.argv[1])

DIR = p.parent / "preproc"
#Make subdirectory if it doesn't exist already
if DIR.is_file():
    raise Exception("The subdirectory exists as a file!")
if not(DIR.is_dir()):
    os.mkdir(DIR)
    print("Created directory", str(DIR))
filename = p.name
filename_without_extension = p.stem
extension = p.suffix #Contains the dot

new_filename = filename_without_extension + "_preproc" + extension
metadata_filename = filename_without_extension + "_metadata.txt"
new_path = DIR / new_filename
metadata_path = DIR / metadata_filename


with open(p, mode='r') as in_file:
    first_line = in_file.readline() #Includes newline
    out_file = open(new_path, "wb")
    met_file = open(metadata_path, "w")
    bytes_written = out_file.write(first_line.encode("ascii"))
    met_file.write("0 " + first_line)
    pos_counter = bytes_written
    for line in in_file: #Includes newline
        if (line.startswith('>')):
            met_file.write(str(pos_counter) + " " + line)
        else:
            line = line.strip() #Remove newline
            pos_counter += out_file.write(line.encode("ascii"))
    out_file.close()
    met_file.close()
