//
// Created by mpelo on 1/13/26.
//
#include <iostream>
#include <string>
#include "fileio.h"
#include "bench.h"
#include <string.h>
#include "errno.h"

void hirgc_compress(FILE *in, FILE *out) {
    BMK_SetByteCompressor(1);
    //FIO_compressor_t compressor = FIO_fse;
    FIO_setDisplayLevel(2);
    FIO_compress_stream(out, in);
}

void compress_bytes(char *buf_in, FILE *out, int n) {
    FILE *in_bytes = fmemopen(buf_in, n, "rb");
    FIO_compress_stream(out, in_bytes);
    fclose(in_bytes);
}

void idocomp_compress(FILE *in, FILE *out) {
    //The fasta line has been handled
    int size_tar, size_m, size_s, size_i;
    if (fread(&size_tar, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_target");
    if (fread(&size_m, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_m");
    if (fread(&size_s, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_s");
    if (fread(&size_i, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_i");
    fwrite(&size_tar, 4, 1, out);
    fwrite(&size_m, 4, 1, out);
    fwrite(&size_s, 4, 1, out);
    fwrite(&size_i, 4, 1, out);
    //Run FSE once on the ints
    size_t n_ints = 2 * size_m + size_s + size_i;
    BMK_SetByteCompressor(1);
    FIO_setDisplayLevel(2);
    char *bytes = new char[n_ints];
    size_t read;
    read = fread(bytes, 1, n_ints, in);
    if (read < n_ints) {
        delete[] bytes;
        throw std::runtime_error("FSE: Error reading first byte sequence");
    }
    compress_bytes(bytes, out, n_ints);
    read = fread(bytes, 1, n_ints, in);
    if (read < n_ints) {
        delete[] bytes;
        throw std::runtime_error("FSE: Error reading second byte sequence");
    }
    compress_bytes(bytes, out, n_ints);
    read = fread(bytes, 1, n_ints, in);
    if (read < n_ints) {
        delete[] bytes;
        throw std::runtime_error("FSE: Error reading third byte sequence");
    }
    compress_bytes(bytes, out, n_ints);
    read = fread(bytes, 1, n_ints, in);
    if (read < n_ints) {
        delete[] bytes;
        throw std::runtime_error("FSE: Error reading fourth byte sequence");
    }
    compress_bytes(bytes, out, n_ints);
    //Now chars
    delete[] bytes;
    size_t n_chars = size_m + size_s + size_i;
    char *chars = new char[n_chars];
    read = fread(chars, 1, n_chars, in);
    if (read < n_chars) {
        delete[] chars;
        throw std::runtime_error("FSE: Error reading char sequence");
    }
    compress_bytes(chars, out, n_chars);
    delete[] chars;
    //Now signs, which is all the way to the end
    FIO_compress_stream(out, in);
}

void hirgc_decompress(FILE *in, FILE *out) {
    FIO_setDisplayLevel(2);
    FIO_decompress_stream(out, in);
}

void idocomp_decompress(FILE *in, FILE *out) {
    int size_tar, size_m, size_s, size_i;
    if (fread(&size_tar, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_target");
    if (fread(&size_m, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_m");
    if (fread(&size_s, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_s");
    if (fread(&size_i, 4, 1, in) != 1)
        throw std::runtime_error("Can't read size_i");
    fwrite(&size_tar, 4, 1, out);
    fwrite(&size_m, 4, 1, out);
    fwrite(&size_s, 4, 1, out);
    fwrite(&size_i, 4, 1, out);
    FIO_setDisplayLevel(2);
    FIO_decompress_stream(out, in);
    FIO_decompress_stream(out, in);
    FIO_decompress_stream(out, in);
    FIO_decompress_stream(out, in);
    FIO_decompress_stream(out, in);//chars
    FIO_decompress_stream(out, in);//signs
}

int main(int argc, char *argv[]) {
    /**
     * Usage:
     * Compress:
     * shuff_fasta -c in_file out_file[no extension]
     * Decompress:
     * shuff_fasta -d in_file[no extension] out_file
     */
    if (strncmp(argv[1], "-c", 2) == 0) {
        //Do not encode FASTA line if it's there
        FILE *in_stream = fopen(argv[2], "rb");
        if (!in_stream) {
            throw std::runtime_error("Can't open " + std::string(argv[2]) + " for reading");
        }
        std::string out_file = argv[3];
        out_file.append(".fse");
        FILE *out_stream = fopen(out_file.c_str(), "wb");
        if (!out_stream) {
            fclose(in_stream);
            throw std::runtime_error("Can't open " + out_file + " for writing");
        }
        //IDoCompCompress vs HiRGC compress
        if (argc >= 5 && strncmp(argv[4], "--idocomp", 9) == 0) {
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
            idocomp_compress(in_stream, out_stream);
        } else {
            hirgc_compress(in_stream, out_stream);
        }
        fclose(in_stream);
        fclose(out_stream);
    } else if (strncmp(argv[1], "-d", 2) == 0) {
        std::string in_file = argv[2];
        in_file.append(".fse");
        FILE *in_stream = fopen(in_file.c_str(), "rb");
        if (!in_stream)
            throw std::runtime_error("Can't open " + in_file + " for reading");
        FILE *out_stream = fopen(argv[3], "wb");
        if (!out_stream) {
            fclose(in_stream);
            throw std::runtime_error("Can't open " + std::string(argv[3]) + " for writing");
        }
        //IDoCompCompress vs HiRGC compress
        if (argc >= 5 && strncmp(argv[4], "--idocomp", 9) == 0) {
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
            idocomp_decompress(in_stream, out_stream);
        } else {
            hirgc_decompress(in_stream, out_stream);
        }
        fclose(in_stream);
        fclose(out_stream);
    } else {
        std::cout << "Usage:\n"
                     "Compress:\n"
                     "fse_hrlz -c in_file out_file[no extension] [--idocomp]\n"
                     "Decompress:\n"
                     "fse_hrlz -d in_file[no extension] out_file [--idocomp]" << std::endl;
        return 1;
    };
}