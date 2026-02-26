//
// Created by maximo on 10-01-26.
//

#include "ShuffEncoder.hpp"
#include "code.h"

#include <iostream>

ShuffEncoder::ShuffEncoder() : Encoder(".shuff", STREAM_VERSION::CPP) {}

void ShuffEncoder::encode_executable(const std::filesystem::path &in_path, const std::filesystem::path &out_path) const {
    std::string cmd = DIR_SHUFF;
    std::string out_string = out_path.string();
    std::string output_with_extension = out_string + extension;
    if (std::filesystem::exists(output_with_extension)) {
        std::cerr << "WARNING: Output of encoding " << out_string << " already exists!!!" << std::endl;
        return;
    }
    cmd.append("/shuff_fasta -c \"").append(std::filesystem::canonical(in_path)).append("\" \"")
    .append(out_path).append("\" >/dev/null 2>/dev/null");
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "WARNING: call to shuff returned exit code " << std::to_string(ret) << std::endl;
    }
    if (!std::filesystem::exists(output_with_extension)) {
        throw std::runtime_error("Output of shuff compression " + output_with_extension + " does not exist!!!");
    }
}

void ShuffEncoder::decode_executable(const std::filesystem::path &in_path, const std::filesystem::path &out_path) const {
    std::string cmd = DIR_SHUFF;
    std::string out_string = out_path.string();
    std::string input_with_extension = in_path.string() + extension;
    if (std::filesystem::exists(out_string)) {
        std::cerr << "WARNING: Output of decoding " << out_string << " already exists!!!" << std::endl;
        return;
    }
    if (!std::filesystem::exists(input_with_extension)) {
        throw std::runtime_error("Input to shuff " + input_with_extension + " does not exist!!!");
    }
    cmd.append("/shuff_fasta -d \"").append(in_path).append("\" \"").append(out_path)
        .append("\" >/dev/null 2>/dev/null");
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "WARNING: call to shuff returned exit code " << std::to_string(ret) << std::endl;
    }
    if (!std::filesystem::exists(out_string)) {
        throw std::runtime_error("Output of shuff decompression " + out_string + " does not exist!!!");
    }
}

void ShuffEncoder::encode_C_stream(FILE *in_stream, const std::filesystem::path &out_path) const {
    std::string path = out_path.string() + extension;
    FILE *out_file = fopen(path.c_str(), "wb");
    encode_C_stream(in_stream, out_file);
    fclose(out_file);
}
void ShuffEncoder::encode_C_stream(FILE* in_stream, FILE* out_file) const {
        //We check if the input stream contains a FASTA definition line. If so, we are going to print it first,
    //without encoding it. Reason: it's not integers, it's random characters. They may be out of range.
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
            throw std::runtime_error("Error reading the input file at shuff");
        }
        //Write it to the output
        fwrite(fasta_line, 1, n_read, out_file);
        free(fasta_line);
    } else {
        //We still write a FASTA line, but empty
        char line[5] = ">fa\n"; //aligned
        fwrite(line, 1, 4, out_file);
    }
    //Write encoded block
    int32_t block_size = DEFAULT_BLOCK_LEN;
    char zeroes_are_EOB = 0; /* false */
    one_pass_encoding(in_stream, out_file, zeroes_are_EOB ? 0 : block_size);
}

void ShuffEncoder::decode_C_stream(const std::filesystem::path &in_path, FILE *out_stream) const {
    std::string path = in_path.string() + extension;
    FILE *in_stream = fopen(path.c_str(), "rb");
    decode_C_stream(in_stream, out_stream);
    fclose(in_stream);
}
void ShuffEncoder::decode_C_stream(FILE* in_stream, FILE* out_stream) const {
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
            throw std::runtime_error("Error reading the input file at shuff");
        }
        fwrite(fasta_line, 1, n_read, out_stream);
    }
    //Now, decode
    do_decoding(in_stream, out_stream);
}
