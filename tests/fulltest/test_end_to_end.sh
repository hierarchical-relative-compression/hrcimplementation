#!/bin/bash
set -x

HRLZ_PATH="../../build/apps/hrlz"

FILE_NAME="small_sars.txt"

DATASET="fasta_sequences"

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
  filenames+=("$(basename "$line")")
  done } < "./${DATASET}/${FILE_NAME}"
echo "${filenames[@]}"

#Make SA files for all the sequences
for i in $(seq 0 $((N - 1)));
do
  ../../build/apps/sa_fasta "./${DATASET}/${filenames[$i]}" "./${DATASET}/${filenames[$i]}.sa"
done

check_sequences () {
    for i in $(seq 0 $((N-1)));
    do
      ${HRLZ_PATH} "./${DATASET}/${FILE_NAME}" -d "./${DATASET}/comp_${COMPRESSOR}" -s "${i}" -o "./${DATASET}/decomp_seqs" --compressor "${COMPRESSOR}" --random 219159270159650 &&
      ../../build/tests/compare_fasta "./${DATASET}/${filenames[$i]}" "./${DATASET}/decomp_seqs/${filenames[$i]}";
      status=$? &&
      rm "./${DATASET}/decomp_seqs/${filenames[$i]}" &&
      if [ $status -ne 0 ]; then echo "Failed decompression of sequence ${i} with compressor ${COMPRESSOR}" 1>&2 ; exit 1 ; fi
    done
}

decompress_and_test () {
    ${HRLZ_PATH} "./${DATASET}/${FILE_NAME}" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_decomp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_decomp.txt" &&
    cd .. &&
    python3 compare_file.py "./fulltest/${DATASET}/${FILE_NAME}" "./fulltest/${DATASET}" "./fulltest/${DATASET}/decomp_${COMPRESSOR}";
    status=$?;
    rm -rf "./fulltest/${DATASET}/decomp_${COMPRESSOR}";
    if [ $status -ne 0 ]; then echo "Failed decompression of dataset ${DATASET} with compressor ${COMPRESSOR}" 1>&2 ; exit 1 ; fi &&
    cd fulltest;
}

run_experiment_second () {
    export OMP_NUM_THREADS="$2"
    #Compress
    ${HRLZ_PATH} "./${DATASET}/$1" -c "./${DATASET}/comp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --load_graph "./${DATASET}/comp_hirgc/graph.txt" --bottom_k --jaccard --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_comp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_comp.txt";
    status=$? &&
    if [ $status -ne 0 ]; then echo "Failed compression of dataset ${DATASET} with compressor ${COMPRESSOR}" 1>&2 ; exit 1 ; fi;
    ${HRLZ_PATH} "./${DATASET}/${FILE_NAME}" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_seqs" --test_sequences 0.5 --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_seqdec.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_seqdec.txt";
    status=$? &&
    if [ $status -ne 0 ]; then echo "Failed sequence decompression test of dataset ${DATASET} with compressor ${COMPRESSOR}" 1>&2 ; exit 1 ; fi;
    check_sequences &&
    decompress_and_test;
}


${HRLZ_PATH} "./${DATASET}/${FILE_NAME}" -c "./${DATASET}/comp_${COMPRESSOR}" --compressor "${COMPRESSOR}" --print_graph --bottom_k --jaccard --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_comp.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_comp.txt";
status=$? &&
if [ $status -ne 0 ]; then echo "Failed compression of dataset ${DATASET} with compressor ${COMPRESSOR}" 1>&2 ; exit 1 ; fi;
${HRLZ_PATH} "./${DATASET}/${FILE_NAME}" -d "./${DATASET}/comp_${COMPRESSOR}" -o "./${DATASET}/decomp_seqs" --test_sequences 0.5 --compressor "${COMPRESSOR}" --random 219159270159650 >"./LOGS/out_${COMPRESSOR}_${DATASET}_seqdec.txt" 2>"./LOGS/err_${COMPRESSOR}_${DATASET}_seqdec.txt";
status=$? &&
if [ $status -ne 0 ]; then echo "Failed sequence decompression test of dataset ${DATASET} with compressor ${COMPRESSOR}" 1>&2 ; exit 1 ; fi;
check_sequences &&
decompress_and_test &&

COMPRESSOR="idocomp" &&
run_experiment_second "${FILE_NAME}" "8" &&

COMPRESSOR="rlz" &&
run_experiment_second "${FILE_NAME}" "8";

status=$?
if [ $status -ne 0 ]; then echo "Test failed somewhere" 1>&2 ; exit 1 ; fi;

#Remove compressed folders
rm -rf "./${DATASET}/comp_hirgc";
rm -rf "./${DATASET}/comp_idocomp";
rm -rf "./${DATASET}/comp_rlz";
rmdir "./${DATASET}/decomp_seqs";

#Remove SA files
for i in $(seq 0 $((N - 1)));
do
  rm "./${DATASET}/${filenames[$i]}.sa"
done
