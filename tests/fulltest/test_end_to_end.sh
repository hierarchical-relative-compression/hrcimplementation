#!/bin/bash

HRLZ_PATH="../../build/apps/hrlz"

FILE_NAME="small_sars.txt"

DATASET="."

COMPRESSOR="hirgc"

export OMP_NUM_THREADS="8"

mkdir -p "./LOGS"

#Read the file, bash-style (yes, it's painful, but here we are)
#Braces to get the input to both commands without spawning a sub-shell; "IFS=" sets the Internal File Separator (the
#delimiter that "read" uses to split the input into words) to nothing, so that it will not split the paths on whitespace
#(should not matter), and -r on read ignores backslashes (escape sequences). The "|| [ -n $line]]" will make it so that
#it still reads the last line, even if it does not end in a newline.
filenames=()
{ read N; while IFS= read -r line || [[ -n $line ]]; do
  filenames+=($line)
  done } < "./${FILE_NAME}"
#echo "${filenames[@]}"

HRLZ_PATH "./${DATASET}/${FILE_NAME}" -c "./${DATASET}/comp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --print_graph --bottom_k --jaccard --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_comp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_comp.txt" &&
HRLZ_PATH "./${DATASET}/${FILE_NAME}" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_seqs" --test_sequences 0.5 --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_seqdec.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_seqdec.txt" &&
for i in $(seq 1 $N);
do
  HRLZ_PATH "./${DATASET}/${FILE_NAME}" -d "./${DATASET}/comp_${COMPRESSOR}" -s $i
done

HRLZ_PATH "./${DATASET}/${FILE_NAME}" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_decomp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_decomp.txt";

run_experiment_second () {
    export OMP_NUM_THREADS="$2"
    #Compress
    /home/maximo/Documents/C++/HRC/hrcimplementation/build/apps/hrlz "./${DATASET}/$1" -c "./${DATASET}/comp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --load_graph "./${DATASET}/comp_hirgc/graph.txt" --bottom_k --jaccard --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_cleanup_comp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_cleanup_comp.txt" &&
    #Decompress
    #hrlz "./${DATASET}/$1" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --random 219159270159650 --dataset "$2" >"./LOGS/out_${COMPRESSOR}_${DATASET}_decomp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_decomp.txt" &&
    #Decompress sequence
    /home/maximo/Documents/C++/HRC/hrcimplementation/build/apps/hrlz "./${DATASET}/$1" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_seqs" --test_sequences 0.5 --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_cleanup_seqdec.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_cleanup_seqdec.txt";
}

DATASET="661k"

COMPRESSOR="idocomp"
run_experiment_second "${FILE_NAME}" "8"


COMPRESSOR="rlz"
run_experiment_second "${FILE_NAME}" "8"