
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include "rlz/rlz.hpp"
#include "rlz/utils.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s {file of filenames}\n", argv[0]);
        exit(1);
    }

    FILE *infilesfile = fopen(argv[1], "r");
    if (!infilesfile) {
        fprintf(stderr, "Error opening file of filenames %s\n", argv[1]);
        exit(1);
    }

    char *filename = (char *) malloc(1024);
    if (!(fgets(filename, 1024, infilesfile))) {
        fprintf(stderr, "Error reading first filename from file of filenames.\n");
        exit(1);
    }

    filename[strlen(filename) - 1] = 0;

    std::cerr << filename << '\n';

    errno = 0;
    FILE *infile = fopen(filename, "r");
    //FILE *infile = fopen("~/Desktop/Simon/data/themisto_data/genomes64.concat.da.dict.16K.1K", "r");
    if (!infile) {
        fprintf(stderr, "Error opening file of base sequence %s, errno=%d\n", filename,errno);
        exit(1);
    }
    fprintf(stderr, "About to read ref\n");

    filelength_type n = 0;
    fseek(infile, 0, SEEK_END);
    n = ftell(infile) / sizeof(data_type);
    std::cerr << "n = " << n << '\n';
    fseek(infile, 0, SEEK_SET);
    DynArray<data_type> x(n+1);
    if (n != fread(x.begin(), sizeof(data_type), n, infile)) {
        fprintf(stderr, "Error reading %lu bytes from file %s\n", static_cast<long unsigned int>(n), filename);
        exit(1);
    }
    x[n] = 0;
    fclose(infile);

    fprintf(stderr, "About to read SA of ref\n");
    //read the suffix array
    char safilename[256];
    sprintf(safilename, "%s.sa", filename);
    std::cerr << safilename << '\n';
    infile = fopen(safilename, "r");
    if (!infile) {
        fprintf(stderr, "Error opening suffix array of input file %s\n", safilename);
        exit(1);
    }
    unsigned int san = 0;
    fseek(infile, 0, SEEK_END);
    san = ftell(infile) / sizeof(unsigned int);
    std::cerr << "san = " << san << '\n';
    fseek(infile, 0, SEEK_SET);
    DynArray<unsigned int> sa(san);
    if (san != fread(sa.begin(), sizeof(unsigned int), san, infile)) {
        fprintf(stderr, "Error reading sa from file\n");
        exit(1);
    }
    fclose(infile);

    //std::cerr << "Checking SA\n";
    //uint64_t lcpsum = 0;
    //for(uint32_t i = 1; i < san; i++){
    //   if((i%10000000) == 0) std::cerr << "Checked: " << i << '\n';
    //   uint32_t s1 = sa[i-1];
    //   uint32_t s2 = sa[i];
    //   //std::cerr << s1 << " " << s2 << '\n'; 
    //   for(uint32_t j=0;j<n;j++){
    //      //std::cerr << x[s1+j] << " " << x[s2+j] << '\n'; 
    //      if(x[s1+j] != x[s2+j]){
    //         if(x[s1+j] > x[s2+j]){
    //            std::cerr << "Suffixes out of order: " << i << '\n';
    //            exit(1);
    //         }
    //         lcpsum+=j;
    //         break;
    //      }
    //   }
    //}
    //std::cerr << "Avg. LCP: " << lcpsum/n << "\n";

    fprintf(stderr, "Reference (size = %lu):\n\t", static_cast<long unsigned int>(n));
    //for (int bi = 0; bi < n; bi++) {
    //    fprintf(stderr, "%lu, ", x[bi]);
    //}
    fprintf(stderr, "\n");

    //compute relative LZ factorization
    RLZCompress compressor(std::move(x), std::move(sa));
    int seqno = 1;
    FactorSink writer;
//    double totalBitsOut = 0;
    while (fgets(filename, 1024, infilesfile)) {
        filename[strlen(filename) - 1] = '\0';
        fprintf(stderr, "Reading sequence %d (%s)\n", seqno, filename);
        
        fprintf(stderr, "---Factorizing sequence %d\n", seqno);
//        double tlz = getTime();
        compressor.compress(filename, writer);
//        fprintf(stderr, "---Time to lz factorize sequence %d = %.2f secs\n", seqno, getTime() - tlz);
//        fprintf(stdout, "%.2f,%lu,%lu,", getTime() - tlz, writer.factor_count(), n);
//        totalBitsOut += bits_out;
        seqno++;
        
    }
//    fprintf(stderr, "----Collection parsed into %lu factors.\n", totalNumFactors);
//    fprintf(stderr, "----Total time: %.2f secs.\n", getTime() - ttotal);
//    fprintf(stderr, "----Estimated encoded size (minus ref): %.2f megabytes.\n",
//            (totalNumFactors * (ceil(log(n) / log(2))) +
//             totalNumFactors * ceil(log(totalCollectionSize / totalNumFactors) / log(2))) / 8 / (1 << 20));
//    fprintf(stderr, "----Estimated encoded size (incl. ref): %.2f megabytes.\n",
//            (2 * n + totalNumFactors * (ceil(log(n) / log(2))) +
//             totalNumFactors * ceil(log(totalCollectionSize / totalNumFactors) / log(2))) / 8 / (1 << 20));
//    fprintf(stderr, "----Size using Elias Delta (minus ref): %.2f megabytes.\n", (totalBitsOut) / 8 / (1 << 20));
//    fprintf(stderr, "----Size using Elias Delta (incl. ref): %.2f megabytes.\n",
//            (2 * n + totalBitsOut) / 8 / (1 << 20));
    exit(0);

    free(filename);
    return 0;
}
