#!/bin/bash
FILE_NAME="small_sars.txt"
filenames=()
{ read n; while IFS= read -r line || [[ -n $line ]]; do
  filenames+=($line)
  done } < "./${FILE_NAME}"
echo "${filenames[@]}"
