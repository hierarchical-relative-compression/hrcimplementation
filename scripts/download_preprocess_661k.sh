#!/bin/bash
for i in {0..9}; do
  rsync -avp "rsync://ftp.ebi.ac.uk/pub/databases/ENA2018-bacteria-661k/Assemblies/batch_00$i" .
done
for i in {10..29}; do
  rsync -avp "rsync://ftp.ebi.ac.uk/pub/databases/ENA2018-bacteria-661k/Assemblies/batch_0$i" .
done

#Move all files from the batch folders to the current directory
find . -type f -name "*.fa.gz" -exec sh -c 'NAME=$(basename $1); mv {} ./$NAME' sh {} \;
#Remove batch folders
find . -mindepth 1 -type d -exec rmdir {} \; 2>/dev/null

#Decompress everything
find . -maxdepth 1 -type f -name "*.fa.gz" -exec gzip -d {} \;

#Preprocess
find . -maxdepth 1 -type f -name "*.fa" -exec python3 many_to_one_fasta.py {} \;

#Remove unprocessed files
find . -maxdepth 1 -type f -name "*.fa" -exec rm {} \;
