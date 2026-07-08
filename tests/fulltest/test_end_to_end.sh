#!/bin/bash

HRLZ_PATH="../../build/apps/hrlz"

DATASET="."

COMPRESSOR="hirgc"

export OMP_NUM_THREADS="8"

mkdir -p "./LOGS"

HRLZ_PATH "./${DATASET}/small_sars.txt" -c "./${DATASET}/comp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --print_graph --bottom_k --jaccard --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_comp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_comp.txt" &&
HRLZ_PATH "./${DATASET}/small_sars.txt" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_seqs" --test_sequences 0.5 --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_seqdec.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_seqdec.txt" &&
HRLZ_PATH "./${DATASET}/small_sars.txt" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_decomp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_decomp.txt";

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
run_experiment_second "abs_list.txt" "8"


COMPRESSOR="rlz"
run_experiment_second "abs_list.txt" "8"