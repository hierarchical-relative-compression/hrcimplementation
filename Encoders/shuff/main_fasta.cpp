//
// Created by mpelo on 1/13/26.
//

#include <iostream>
#include <stdexcept>
#include <string>
#include <string.h>
#include "code.h"

void print_usage() {
    const char *usage = R"(Usage:
        Compress:
        shuff_fasta -c in_file out_file[no extension]
        Decompress:
        shuff_fasta -d in_file[no extension] out_file)";
    std::cout << usage << std::endl;
    std::exit(1);
}

int main(int argc, char *argv[]) {
    /**
     * Usage:
     * Compress:
     * shuff_fasta -c in_file out_file[no extension]
     * Decompress:
     * shuff_fasta -d in_file[no extension] out_file
     */
    if (argc < 4) {
        print_usage();
    }
    if (strncmp(argv[1], "-c", 2) == 0) {
        //Do not encode FASTA line if it's there
        FILE *in_stream = fopen(argv[2], "rb");
        if (!in_stream)
            throw std::runtime_error("Can't open " + std::string(argv[2]) + " for reading");
        std::string out_file = argv[3];
        out_file.append(".shuff");
        FILE *out_stream = fopen(out_file.c_str(), "wb");
        if (!out_stream) {
            fclose(in_stream);
            throw std::runtime_error("Can't open " + out_file + " for writing");
        }
        //Read FASTA line if it's there
        int c;
        c = fgetc(in_stream);
        ungetc(c, in_stream);
        if (c == '>') {
            //Read the FASTA line
            char *fasta_line = nullptr;
            size_t alloc_len = 0;
            ssize_t n_read = getline(&fasta_line, &alloc_len, in_stream);
            if (n_read == -1) {
                free(fasta_line);
                fclose(in_stream);
                fclose(out_stream);
                throw std::runtime_error("Error reading the input file at shuff");
            }
            //Write it to the output
            fwrite(fasta_line, 1, n_read, out_stream);
            free(fasta_line);
        } else {
            //We still write a FASTA line, but empty
            char line[5] = ">fa\n"; //aligned
            fwrite(line, 1, 4, out_stream);
        }
        //Encode the rest
        int32_t block_size = DEFAULT_BLOCK_LEN;
        char zeroes_are_EOB = 0; /* false */
        one_pass_encoding(in_stream, out_stream, zeroes_are_EOB ? 0 : block_size);
        fclose(in_stream);
        fclose(out_stream);
    } else if (strncmp(argv[1], "-d", 2) == 0) {
        std::string in_file = argv[2];
        in_file.append(".shuff");
        FILE *in_stream = fopen(in_file.c_str(), "rb");
        if (!in_stream)
            throw std::runtime_error("Can't open " + in_file + " for reading");
        FILE *out_stream = fopen(argv[3], "wb");
        if (!out_stream) {
            fclose(in_stream);
            throw std::runtime_error("Can't open " + std::string(argv[3]) + " for writing");
        }
        //Peek to see if there's a FASTA line that's written directly
        int c;
        c = fgetc(in_stream);
        ungetc(c, in_stream);
        if (c == '>') {
            //Read the FASTA line
            char *fasta_line = nullptr;
            size_t alloc_len = 0;
            ssize_t n_read = getline(&fasta_line, &alloc_len, in_stream);
            if (n_read == -1) {
                free(fasta_line);
                fclose(in_stream);
                fclose(out_stream);
                throw std::runtime_error("Error reading the input file at shuff");
            }
            fwrite(fasta_line, 1, n_read, out_stream);
        }
        //Decode
        do_decoding(in_stream, out_stream);
        fclose(in_stream);
        fclose(out_stream);
    } else {
        std::cout << "Usage:\n"
                     "Compress:\n"
                     "shuff_fasta -c in_file out_file[no extension]\n"
                     "Decompress:\n"
                     "shuff_fasta -d in_file[no extension] out_file" << std::endl;
        return 1;
    };
    return 0;
}