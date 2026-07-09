Contact Máximo Pérez López (mpelo@dtu.dk, maxiplopez@gmail.com) for support.
# Updates

06-07-2026: Removed legacy programs
07-07-2026: Added standalone compilations for hirgc_exec, idocomp_exec and rlz_exec

# Contents

This repository contains all the code and scripts used in the paper "From Relative Compression to Hierarchical Compression" by Philip Bille, Inge Li Gørtz and Máximo Pérez López. The repository includes the sources for the main program that computes relative or hierarchical compression with any of the three relative compression algorithms discussed in the paper (HiRGC, iDoComp and RLZ). It also contains the sources of the custom, stand-alone re-implementations of these three compression algorithms, plus the programs to compute suffix arrays.

Compiling the sources as indicated below will produce binaries in the sub-folder "apps". The relevant binaries are:
- hrlz: the program that computes relative or hierarchical compressions of datasets.
- hirgc_max_exec: stand-alone re-implementation of the HiRGC relative compressor.
- iDoCompMaxExec: stand-alone re-implementation of the iDoComp relative compressor.
- rlz_exec: stand-alone implementation of RLZ.
- compress_hierarchy: compresses a dataset according to the hierarchy specified in a "metadata" file (useful for \[re-\]compressing datasets if the hierarchy has already been computed).
- largest_path: outputs the length of the longest root-to-leaf path in a hierarchy.

Below are instructions on how to compile the source, how to use each of the binaries, and instructions on how to pre-process genomic datasets so that they are usable with this software. Credits are at the end.
# How to compile

The script `cmake_compile.sh` will compile all the code in this repository under a sub-directory "build". It requires C++20. The code compiles in a standard, recent version of Linux. Due to time constraints, unfortunately no attempt was made to make the codebase portable to other OS such as Mac or Windows. Apologies from the author. 

The alternative script `cmake_compile_arch.sh` will compile the code in Release mode targeting the native architecture of the PC running the compile script. The alternative script `cmake_compile_debug.sh` compiles the code in debug mode, without optimizations. This version can be useful for debugging.

# How to use

### HRLZ (Hierarchical compression)

The project provides several binaries. The main one is `hrlz`, which given a list of FASTA files, by default it compresses them by computing an approximate hierarchy, using one of three implemented relative compressors, and returns the compressed size. The command line format is:

```
hrlz path/to/file_list.txt [options...]
```

The file containing the list of files to compress should contain a first line with a number, indicating how many sequences are in the dataset, followed by paths to the files. All the files should be in the same folder. It should look something like the following:

```
3
/home/joe/data/sequence1.fa
/home/joe/data/sequence2.fa
/home/joe/data/sequence3.fa
```

*Tip: navigating to your data folder and typing 
`find "$PWD" . -maxdepth 1 -type f >file_list.txt`
will write the absolute paths of all files in the current directory to the file `file_list.txt`. Writing manually the number on top should give a correctly formatted list of files.*

The available options for `hrlz` are:
- `-c directory`: saves the compressed database in the specified directory. If not specified, the program will only print to the standard output the size of the compression.
- `-d directory`: decompresses the database located in the specified directory, instead of compressing. 
- `-o directory`: If decompressing (setting `-d` above), selects the specified directory for the decompressed output.
- `-s index`: If decompressing (setting `-d` above), decompresses only the sequence indexed by `index` (0-based). 
- `--rlz root_id`: Instead of compressing with hierarchical compression, compresses with relative compression with the specified root (`root_id` is the 0-based index of the desired root in the list of sequences provided as the first argument)
- `--test_rels perc`: Tests a random sample of `perc%` of the sequences as roots of relative compression, and outputs the size of the best one (or stores the compressed database, if `-c` was used). Example: `--test_rels 0.5` tests a 0.5% of all the relative compressions.
- `--compressor hirgc|idocomp|rlz` : chooses HiRGC, iDoComp or RLZ as the underlying relative compression algorithm. The default is `rlz`.
- `--print_graph`: Will print the sparsified graph that was computed using the fingerprints. It will be stored as a file `graph.txt`, that can be loaded with the next option.
- `--load_graph path/to/graph.txt`: loads a specific graph to be used for the hierarchy computation, instead of computing one from scratch.
- `--sketch_size q`: sets the fingerprint size to `q` (`q` different hash functions will be used to compute the min-hash). The default is 4.
- `--random seed`: uses the number `seed` as the global random seed for all the random number generators.
- `--bottom_k`: Uses bottom-k hashing for the fingerprint instead of min-k hashing.

The options may be written in any order. An example execution can be:

`hrlz ./file_list.txt -c ./output-comp/ --compressor hirgcmax --print_graph --sketch_size 1`

An extra file named `metadata` should appear in the compressed directory, along with all the compressed files. To decompress:

`hrlz ./file_list.txt -d ./output-comp/ -o ./output-decomp/ --compressor hirgcmax`

If we wish to decompress the 2nd file in the dataset (ordered by the appearance in `file_list.txt`), then we would run:

`hrlz ./file_list.txt -d ./output-comp/ -s 1 -o ./output-decomp/ --compressor hirgcmax`

This execution will attempt to read the `metadata` file located in `output-comp`, and will fail if it is not found.

To perform relative compression instead, testing 0.5% of the possible roots:

`hrlz ./file_list.txt -c ./output-comp/ --compressor hirgcmax --test_rels 0.5`

### HiRGC, iDoComp and RLZ

We provide standalone implementations of the relative compressors HiRGC, iDoComp and RLZ that we use as part of the hierarchical relative compression package. Using a general compilation script (`cmake_compile.sh` and its other versions) we will find the binaries `hirgc_exec`, `idocomp_exec` and `rlz_exec` in the `build/apps` folder. Alternatively, we can choose to compile just these with the `hirgc_compile.sh`, `idocomp_compile.sh` or `rlz_compile.sh` scripts.

To compress with `hirgc_exec` or `idocomp_exec`, we follow this command:

`hirgc_exec -c reference_file target_file output_dir`

where `reference_file` is the path to the reference, `target_file` is the path to a single target to compress, and `output_dir` is the output directory where the compressed target will be stored. The program allows relative paths too.

To decompress, use the command:

`hirgc_exec -d reference_file target_file output_dir [target_original_filename]`

where `reference_file` is the uncompressed reference (the same file as when compressing), `target_file` is the compressed target, `output_dir` is the output directory for the decompressed target, and `target_original_filename` is an optional parameter specifiying the name of the output file.

When using `idocomp_exec` or `rlz_exec`, the interface is the same. However, note that both iDoComp and RLZ require the suffix array of the reference to be available for parsing. The suffix array must be stored in a file with the exact same name as `reference_file` plus the extension `.sa`, and in the same directory as the reference. To construct the suffix array of a file, one can use the program `sa_fasta` (built automatically in `build/apps/` when executing `idocomp_exec` or `rlz_exec`). The following command will create the required suffix array for the reference:

`sa_fasta reference_file reference_file.sa`

The program `sa_fasta` essentially runs *divsufsort* on `reference_file` but skips the first FASTA definition line, so that it doesn't pollute the suffix array.
 
### compress_hierarchy

Given a metadata file containing a hierarchy for a dataset, we provide a utility to compress again the dataset following the hierarchy. This can be useful if the last stage of the compression failed, or if we wish to compress the dataset again, following the same hierarchy, but with a different compressor.

The tool is located in `build/apps/compress_hierarchy`, and it has the following interface:
```
compress_hierarchy [path to directory containing metadata] -o [output directory] [--compressor hirgc|idocomp|rlz]
```
The metadata file must be named `metadata` without extensions.

# Pre-processing of datasets

All the relative compressors of this project work on FASTA files. The FASTA files should have one first line starting with the character '>' (the FASTA definition line), ended by a newline character '\n', and followed by all the DNA data **in one line**. In particular:
- FASTA files that contain assemblies of several sequences, each with their own FASTA definition line, must be split into different files, each containing only one sequence with its unique definition line.
- FASTA/FASTQ files containing draft assemblies, contigs or scaffolds of one sequence with a definition line for each DNA subsequence must be pre-processed to remove all FASTA definition lines except for the first one, and concatenating all DNA subsequences into one single line.

We provide utilities to perform this preprocessing (the ones we personally used in the project). The following programs are under the folder `scripts`:
- fasta_conv_new.py: to preprocess FASTA files that contain several full assemblies with their own FASTA defintion line.
```
$ fasta_conv_new.py <file.fa
```
The program will create a sub-directory `fasta_sequences` where all the produced FASTA files will be stored, in the required one-line format. Their file name will be the first word in the FASTA definition line. Care should be taken in terms of name conflicts if several files have the same first word as part of the FASTA definition line, or if this word contains special characters. 

- many_to_one_fasta.py: to preprocess FASTA/FASTQ files that contain draft assemblies, contigs or scaffolds as explained before. 
```
$ many_to_fasta.py /path/to/file.fa
```
The program will create a sub-directory `preproc` where two files `file_preproc.fa` and `file_metadata.txt` will be created. The former contains the first FASTA definition line plus all the DNA data compacted in one line, ready to use with our compression tools. The latter contains all the other FASTA definition lines that were extracted from the input file, together with the index where they would be inserted in `file_preproc.fa` to recover the original file.

Finally, to specify newline characters we use the letter 'N' (we allow both lower-case and upper-case). In case the dataset contains dashes (`-`) instead of Ns, the script `change_dashes_to_Ns.sh` does the substitution:
```
change_dashes_to_Ns.sh file.fa
```

The program will do an in-place substitution. It is implemented with a simple `sed` command:
`sed -i 's/-/N/g' $1` .
# Credits

The main code for the hierarchical compression was written by Simon Rumle Tarnow. The re-implementations of HiRGC and iDoComp were written by Máximo Pérez López, as well as the glue code to the hierarchical compression and the bottom-k hashing and Jaccard similarity optimizations. The RLZ code was written by Simon J. Puglisi. 

