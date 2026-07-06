//Introduction
    This is a referential genome compression algorithm for assembled genome data sequences in which different matching strategies are used depending on the similarity between the reference and the target sequences.

    This package contains four .java files for compression (SCCGC.java, ORGC.java) and decompression (SCCGD.java, ORGD.java).
    SCCGC.java and SCCGD.java are used for a single sequence.
    ORGC.java and ORGD.java are used for a whole human genome.
    They have passed our test on Ubantu system, and can be used directly on Ubantu system.

//Compile
    Frist, please execute "makefile.sh" to compile the source codes.

//Compression for a single sequence
   You can compress a genomic sequence by executing a command like this:
java -Xms8192m SCCGC ./genome/hg18/chr19.fa ./genome/hg19/chr19.fa ./result

   The command line contains five parameters: 
   -the option "-Xms8192m" is the initial allocation of memory;
   -the 2nd parameter is the encoder itself.
   -the 3rd parameter "./genome/hg18/chr19.fa" is the path name of the reference sequence file, please make sure this parameter is the path name of a file of .fa format;
   -the 4th parameter "./genome/hg19/chr19.fa" is the path name of the target sequence, please make sure this parameter is the path name of a file of .fa format;
   -the 5th parameter "./result" is the path name of the output folder.

//Decompression for a single sequence
    You can decompress a compressed file of a genomic sequence by executing a command like this: 
java -Xms8192m SCCGD ./genome/hg18/chr19.fa ./result/chr19.fa.7z ./dresult
   
   The command line also has five parameters: 
   -the option "-Xms8192m" is the initial allocation of memory;
   -the 2nd parameter is the decoder itself.   
   -the 3rd parameter "./genome/hg18/chr19.fa" is the path name of the reference sequence file, please make sure this parameter is the path name of a file of .fa format;
   -the 4th parameter "./result/chr19.fa.7z" is the path name of the compressed file, please make sure this parameter is the path name of a file of .7z format;
   -the 5th parameter "./dresult" is the path name of the output folder.

//Notice
Please note the following points to successfully run the program:
	1. Please make sure your machine has more than 8GB physical memory (better with 10GB), please set the initial memory of JVM to more than 8GB, and you need to include the "-Xms8192m" option in the command line; 
	2. Please ensure that the path separator entered in the command line is "/", and the path name to the input file is recommended as the absolute path;
	3. Please ensure that the 7zip tool path is set up correctly, we assume that the path name is "./7za" under the Linux system. 
	4. For compression, ensure that the input file names are not with suffix ".7z";
	5. For decompression, ensure that the compressed file name is with suffix ".7z", and only one ".7z" string can be found in the filename;
	6. It is not recommended that input and output files are put in the same directory.

//Compression and decompression for an entire human genome folder
　　The calling conventions of ORGC and ORGD are similar to that of the SCCGC and SCCGD. 
　　
　　For the compression of a human genome folder to a single .7z file, the command line looks like:
java -Xms8192m -Xmx10g ORGC ./genome/hg18 ./genome/hg19 ./result
　  -the 1st option "-Xms8192m" is the initial allocation of memory;
　  -the 2nd option "-Xmx10g" is the maximum allowed memory requirement;
    -the 3rd parameter is the encoder itself.  
　  -the 4th parameter "./genome/hg18" is the path name of the reference folder.
    -the 5th parameter "./genome/hg19" is the path name of the target folder.
　  -the 6th parameter "./result" is the path name of the output folder
　　
　　For decompression from a single .7z file to a human genome folder, the command line can be: 
java -Xms8192m -Xmx10g ORGD ./genome/hg18 ./result/result.7z ./dresult
　  -the 1st option "-Xms8192m" is the initial allocation of memory;
　  -the 2nd option "-Xmx10g" is the maximum allowed memory requirement;
    -the 3rd parameter is the decoder itself.  
　  -the 4th parameter "./genome/hg18” is the path name of the reference folder
    -the 5th parameter "./result/result.7z" is the path name of the compressed file.
　  -the 6th parameter "./dresult" is the path name of the output folder

//Notice
1. If you want to compress an entire human genome folder directly using our code, make sure that there are 24 chromosome files in the input folder with the following names:"chr1.fa", "chr2.fa", "chr3.fa", "chr4.fa", "chr5.fa", "chr6.fa", "chr7.fa", "chr8.fa", "chr9.fa", "chr10.fa", "chr1 1.fa", "chr12.fa", "13.fa", "chr14.fa", "chr15.fa", "chr16.fa". , "chr17.fa", "chr18.fa", "chr19.fa", "chr20.fa", "chr21.fa", "chr22.fa", "chrX.fa", "chrY.fa". For the compression of a non-human genome, you have to modify the main class in the source code.
　　2. By default, we named the compressed genome file "result.7z".

//Contacts
    If you have any questions regarding our algorithm, please contact me at chenjh@ynu.edu.cn  .
