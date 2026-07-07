//
// Created by mpelo on 12/20/25.
//

#include "HuffmanFile.hpp"
#include "huf.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "fileio.h"
#include "bench.h"


void write_metadata(const std::filesystem::path& file, size_t string_size) {
    std::ofstream metadata(file.string() + ".met", std::ios_base::binary | std::ios_base::out);
    if (!metadata)
        throw std::runtime_error("Couldn't open " + file.string() + ".met for writing!");
    if (!metadata.write(reinterpret_cast<char*>(&string_size), sizeof(string_size)))
        throw std::runtime_error("Couldn't write to " + file.string() + ".met");
    metadata.close();
}

size_t metadata_read(const std::filesystem::path& file) {
    std::ifstream metadata(file.string() + ".met", std::ios_base::binary | std::ios_base::in);
    size_t size;
    metadata.read(reinterpret_cast<char*>(&size), sizeof(size));
    return size;
}

size_t huffman_compress(const std::string& str, const std::filesystem::path& output_file) {
    size_t weight = 0;
    size_t source_size = str.size();
    if (source_size > HUF_BLOCKSIZE_MAX) {
        //We need to serialize to file, and call the executable
        std::ofstream out_raw(output_file, std::ios_base::binary | std::ios_base::out);
        if (!out_raw)
            throw std::runtime_error("Couldn't open " + output_file.string() + " for writing!!!");
        if (!out_raw.write(str.c_str(), str.size()))
            throw std::runtime_error("Couldn't write to " + output_file.string());
        out_raw.close();

        //Call the executable
        std::string cmd = DIR_FSE;
        cmd.append("/fse_exec -h \"").append(output_file.string()).append("\" \"").append(output_file.string().append(".huf\""));
        int ret = system(cmd.c_str());
        if (ret != 0)
            throw std::runtime_error("Huffman compression returned bad error code: " + std::to_string(ret));

        //Delete the leftover file
        std::filesystem::remove(output_file);
        //Huffman encoding with the executable will always run, even if it is not possible to compress the file, or
        //we get a bigger file.
        write_metadata(output_file, source_size);
        weight = std::filesystem::file_size(output_file.string().append(".huf"));
    } else {
        //We can compress in-memory, but we have to check for incompressibility
        size_t dest_size = HUF_compressBound(source_size);
        char *comp_buf = new char[dest_size];
        weight = HUF_compress(comp_buf, dest_size, str.c_str(), source_size);
        size_t metadata_size;
        const char *ptr_to_write;
        if (weight == 0) {
            //The string was incompressible. We write the string as is
            ptr_to_write = str.c_str();
            metadata_size = -1UL; //We signify that there's no compression with size -1
            weight = source_size;
        } else {
            //The string was compressed fine
            ptr_to_write = comp_buf;
            metadata_size = source_size;
        }
        std::ofstream out(output_file.string().append(".huf"));
        if (!out)
            throw std::runtime_error("Couldn't open " + output_file.string().append(".huf"));
        if(!out.write(ptr_to_write, weight))
            throw std::runtime_error("Couldn't write to " + output_file.string() + ".huf");
        out.close();
        write_metadata(output_file, metadata_size);
        delete[] comp_buf;
    }
    return weight;
}
void huffman_decompress(const std::filesystem::path& compressed_file, const std::filesystem::path& output) {
    std::filesystem::path comp_file = compressed_file.parent_path() / (compressed_file.filename().string().append(".huf"));
    //First, read metadata to get form of compression & original size
    size_t original_size = metadata_read(compressed_file);
    if (original_size == -1UL) {
        //The file was not compressed (was incompressible), copy as is
        std::filesystem::copy(comp_file, output);
    } else {
        if (original_size > HUF_BLOCKSIZE_MAX) {
            //Decompress with the executable
            std::string cmd = DIR_FSE;
            cmd.append("/fse_exec -d \"").append(comp_file.string()).append("\" \"").append(output.string()).append("\"");
            int ret = system(cmd.c_str());
            if (ret != 0)
                throw std::runtime_error("Decompression with Huffman returned bad code: " + std::to_string(ret));
            if (!std::filesystem::exists(output))
                throw std::runtime_error("Decompression with Huffman didn't produce output at: " + output.string());
        } else {
            //Decompress with function, have to read & write
            size_t comp_size = std::filesystem::file_size(comp_file);
            std::ifstream in_stream(comp_file, std::ios_base::binary | std::ios_base::in);
            if (!in_stream) {
                throw std::runtime_error("Couldn't open file " + comp_file.string());
            }
            char *in = new char[comp_size];
            if (!in_stream.read(in, comp_size)) {
                delete[] in;
                throw std::runtime_error("Couldn't read from file " + comp_file.string());
            }
            size_t out_buffer_size = HUF_compressBound(original_size);
            char *out = new char[out_buffer_size];
            size_t weight = HUF_decompress(out, original_size, in, comp_size);
            if (HUF_isError(weight)) {
                delete[] in;
                delete[] out;
                throw std::runtime_error("Huffman decompress error:" + std::string(HUF_getErrorName(weight)));
            }
            //Write file
            std::ofstream out_stream(output, std::ios_base::binary | std::ios_base::out);
            if (!out_stream) {
                delete[] out;
                delete[] in;
                throw std::runtime_error("Couldn't open " + output.string() + " for writing");
            }
            if (!out_stream.write(out, original_size)) {
                delete[] out;
                delete[] in;
                throw std::runtime_error("Couldn't write decompression to " + output.string());
            }
            out_stream.close();
            delete[] out;
            delete[] in;
        }
    }
}

HuffmanEncoder::HuffmanEncoder() : Encoder(".fse", STREAM_VERSION::CPP) {}
HuffmanEncoder::~HuffmanEncoder() {}

void HuffmanEncoder::decode_executable(const std::filesystem::path& in_path, const std::filesystem::path& out_path) const {
    std::string cmd = DIR_FSE;
    std::string out_string = out_path.string();
    if (std::filesystem::exists(out_path)) {
        std::cerr << "WARNING: Output of decompress Huffman" + out_string + "  already exists!!! Ignoring Huffman" << std::endl;
        return;
    }
    cmd.append("/fse_hrlz -d \"").append(in_path.string()).append("\" \"")
            .append(out_string).append("\" ");
    if (idocomp) {
        cmd.append("--idocomp ");
    }
    cmd.append(">/dev/null 2>/dev/null");
    //std::cout << "Calling Huffman decoder with " << cmd << std::endl;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "WARNING: Call to Huffman decode returned exit code " << ret << std::endl;
    }
    if (!std::filesystem::exists(out_path))
        throw std::runtime_error("Output of fse decompression " + out_string + " does not exist!!!");
}

void HuffmanEncoder::encode_executable(const std::filesystem::path &in_path,
                                       const std::filesystem::path &out_path) const {
    std::string cmd = DIR_FSE;
    std::string output_with_extension = (out_path.parent_path() / (out_path.filename().string() + ".fse")).string();
    if (std::filesystem::exists(output_with_extension)) {
        std::cerr << "WARNING: Output of Huffman compression " + output_with_extension + " already exists!!! Ignoring Huffman" << std::endl;
        return;
    }
    cmd.append("/fse_hrlz -c \"").append(std::filesystem::canonical(in_path)).append("\" \"")
            .append(out_path).append("\" ");
    if (idocomp) {
        cmd.append("--idocomp ");
    }
    cmd.append(">/dev/null 2>/dev/null");
    //std::cout << "Calling Huffman encoder with " << cmd << std::endl;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "WARNING: Call to fse returned exit code " << ret << std::endl;
    }
    if (!std::filesystem::exists(output_with_extension)) {
        throw std::runtime_error("Output of fse compression " + output_with_extension + " does not exist!!!");
    }
}

void HuffmanEncoder::encode_C_stream(FILE *in_stream, FILE *out_stream) const {
    BMK_SetByteCompressor(1);
    //FIO_compressor_t compressor = FIO_fse;
    FIO_setDisplayLevel(2);
    FIO_compress_stream(out_stream, in_stream);
}

void HuffmanEncoder::decode_C_stream(FILE *in_path, FILE *out_stream) const {
    FIO_setDisplayLevel(2);
    FIO_decompress_stream(out_stream, in_path);
}

