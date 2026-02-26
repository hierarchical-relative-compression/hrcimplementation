
# How to compile

The script `cmake_compile.sh` will compile all the code in this repository under a sub-directory "build". It requires C++20. The code compiles in a standard, recent version of Linux. Due to time constraints, unfortunately no attempt was made to make the codebase portable to other OS such as Mac or Windows. Apologies from the author. 

The alternative script `cmake_compile_arch.sh` will compile the code in Release mode targeting the native architecture of the PC running the compile script. The alternative script `cmake_compile_debug.sh` compiles the code in debug mode, without optimizations. This version can be useful for debugging.

# How to use

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
- `-o directory`: If decompressing (setting -d above), selects the specified directory for the decompressed output.
- `--rlz root_id`: Instead of compressing with hierarchical compression, compresses with relative compression with the specified root (`root_id` is the 0-based index of the desired root in the list of sequences provided as the first argument)
- `--test_rels perc`: Tests a random sample of `perc%` of the sequences as roots of relative compression, and outputs the size of the best one (or stores the compressed database, if `-c` was used). Example: `--test_rels 0.5` tests a 0.5% of all the relative compressions.
- `--compressor hirgcmax|idocompmax|rlz` : chooses HiRGC, iDoComp or RLZ as the underlying relative compression algorithm. The default is `rlz`.
- `--print_graph`: Will print the sparsified graph that was computed using the fingerprints. It will be stored as a file `graph.txt`, that can be loaded with the next option.
- `--load_graph path/to/graph.txt`: loads a specific graph to be used for the hierarchy computation, instead of computing one from scratch.
- `--sketch_size q`: sets the fingerprint size to `q` (`q` different hash functions will be used to compute the min-hash). The default is 4.
- `--random seed`: uses the number `seed` as the global random seed for all the random number generators.
- `--bottom_k`: Uses bottom-k hashing for the fingerprint instead of min-k hashing. Experimental.

The options may be written in any order. An example execution can be:

`hrlz ./file_list.txt -c ./output-comp/ --compressor hirgcmax --print_graph --sketch_size 1`

An extra file named `metadata` should appear in the compressed directory, along with all the compressed files. To decompress:

`hrlz ./file_list.txt -d ./output-comp/ -o ./output-decomp/ --compressor hirgcmax`

This execution will attempt to read the `metadata` file located in `output-comp`, and will fail if it is not found.

To perform relative compression instead, testing 0.5% of the possible roots:

`hrlz ./file_list.txt -c ./output-comp/ --compressor hirgcmax --test_rels 0.5`

# Credits

The main code for the hierarchical compression was written by Simon Rumle Tarnow. The re-implementations of HiRGC and iDoComp were written by Máximo Pérez López, as well as the glue code to the hierarchical compression and the bottom-k hashing and Jaccard similarity optimizations. The RLZ code was written by Simon J. Puglisi. 
