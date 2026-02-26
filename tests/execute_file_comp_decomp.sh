#!/bin/bash
#Compress and decompress the 5 sequences database
#Get the compression type
COMPRESSION_TYPE="$1"
if [ -z "$COMPRESSION_TYPE" ]; then
    COMPRESSION_TYPE="hirgc"
fi
#Get the input and output paths
TEST_DIR=`pwd`
echo "At directory $TEST_DIR"
DATA_DIR="/home/mpelo/DATA_COMPRESSION/Ecoli Simon/ecoli"
OUTPUT_COMPRESSION="${DATA_DIR}/output_dir"
OUTPUT_DECOMPRESSION="${DATA_DIR}/output_decompression"
#Check if directories exist
if [ ! -d "$OUTPUT_COMPRESSION" ]; then
    mkdir "$OUTPUT_COMPRESSION"
fi
if [ ! -d "$OUTPUT_DECOMPRESSION" ]; then
    mkdir "$OUTPUT_DECOMPRESSION"
fi
echo "${TEST_DIR}"
echo "${DATA_DIR}"
echo "${OUTPUT_COMPRESSION}"
echo "${OUTPUT_DECOMPRESSION}"
#Clear the contents of the compression output folders
find "${OUTPUT_COMPRESSION}" -type f -delete
find "${OUTPUT_DECOMPRESSION}" -type f -delete
#rm "${OUTPUT_COMPRESSION}"/*
#rm "${OUTPUT_DECOMPRESSION}"/*
#Now, compress
"${TEST_DIR}/../build/apps/hrlz" "${DATA_DIR}/abs_five.txt" "-c" "${OUTPUT_COMPRESSION}" "--compressor" "$COMPRESSION_TYPE"
#Decompress
"${TEST_DIR}/../build/apps/hrlz" "-d" "${OUTPUT_COMPRESSION}" "-o" "${OUTPUT_DECOMPRESSION}" "--compressor" "$COMPRESSION_TYPE"
#Check
python3 compare_file.py "${DATA_DIR}/abs_five.txt" "${DATA_DIR}/five" "${OUTPUT_DECOMPRESSION}"
