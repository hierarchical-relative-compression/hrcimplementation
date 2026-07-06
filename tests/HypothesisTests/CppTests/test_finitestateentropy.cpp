//
// Created by mpelo on 12/19/25.
//

#include <iostream>
#include <fstream>
#include "test_finitestateentropy.hpp"
#include "huf.h"
#include "HuffmanFile.hpp"
#include "utils_tests.hpp"

int64_t compress_decompress_huffman(const std::string& str) {
    size_t text_size = str.size();
    char *string = new char[text_size + 1];
    str.copy(string, str.size());
    string[text_size] = '\0';
    size_t dest_size = HUF_compressBound(text_size);
    char *dest = new char[dest_size];
    size_t comp_size = HUF_compress(dest, dest_size, string, text_size);
    if (comp_size != 0) {
        if (HUF_isError(comp_size)) {
            std::cerr << "Error at compression:" << std::endl;
            std::cerr << HUF_getErrorName(comp_size) << std::endl;
            return 0;
        }
        //now, decompress
        char *decompressed = new char[dest_size];
        size_t decomp_size = HUF_decompress(decompressed, text_size, dest, comp_size);
        if (HUF_isError(decomp_size)) {
            std::cerr << "Error at decompression: " << std::endl;
            std::cerr << HUF_getErrorName(decomp_size) << std::endl;
            return 0;
        }
        if (decomp_size != text_size) {
            std::cerr << "Wrong decompressed size: original " << comp_size << ", decompressed: " << decomp_size << std::endl;
            return 0;
        }
        for (size_t i = 0; i < text_size; i++) {
            if (str[i] != decompressed[i])
                return i;
        }
        delete[] decompressed;
    } //Else, the string was incompressible, and nothing was copied to dst! But we don't need decompression either.
    delete[] dest;
    delete[] string;
    return -1;
}

int64_t compress_decompress_huffman_api(const std::string& str) {
    size_t str_size = str.size();
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    static int ix = 0;
    std::string filename = "huffman_" + std::to_string(ix++);
    std::filesystem::path comp_file = temp_dir / filename;
    std::filesystem::path decomp_file = temp_dir / (filename + "_dec");
    std::filesystem::path comp_file_ext = temp_dir / (filename + ".huf");
    std::filesystem::path met_file = temp_dir / (filename + ".met");
    size_t comp_size;
    try {
        comp_size = huffman_compress(str, comp_file);
    } catch (std::exception& e) {
        std::cerr << "Error during compression: " << e.what() << std::endl;
        std::filesystem::remove(comp_file_ext);
        std::filesystem::remove(met_file);
        return 0;
    }
    if (comp_size == 0) {
        std::cerr << "Huffman didn't compress at all";
        std::filesystem::remove(comp_file_ext);
        std::filesystem::remove(met_file);
        return 0;
    }
    //Decompress
    try {
        huffman_decompress(comp_file, decomp_file);
    } catch (std::exception& e) {
        std::cerr << "Error at decompression: " << e.what() << std::endl;
        std::filesystem::remove(comp_file_ext);
        std::filesystem::remove(met_file);
        std::filesystem::remove(decomp_file);
        return 0;
    }
    //Read decompressed file
    std::ifstream in(decomp_file, std::ios_base::binary | std::ios_base::in);
    if (!in) {
        std::cerr << "Couldn't open decompressed file to read: " << decomp_file.string() << std::endl;
        std::filesystem::remove(comp_file_ext);
        std::filesystem::remove(met_file);
        std::filesystem::remove(decomp_file);
        return 0;
    }
    char *dec = new char[str.size()];
    if (!in.read(dec, str_size)) {
        std::cerr << "Error reading decompressed file " << decomp_file.string() << std::endl;
        std::filesystem::remove(comp_file_ext);
        std::filesystem::remove(met_file);
        std::filesystem::remove(decomp_file);
        return 0;
    }
    for (size_t i = 0; i < str_size; i++) {
        if (str[i] != dec[i])
            return i;
    }
    delete[] dec;
    std::filesystem::remove(comp_file_ext);
    std::filesystem::remove(met_file);
    std::filesystem::remove(decomp_file);
    return -1;
}

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

int64_t compress_decompress_fse_mem(const std::string& str) {
    char *buf_in = const_cast<char*>(str.c_str());
    char *buf_out = nullptr;
    size_t size_out;
    FILE *in = fmemopen(buf_in, str.size(), "rb");
    FILE *out = open_memstream(&buf_out, &size_out);
    HuffmanEncoder huff;
    huff.encode_C_stream(in, out);
    fclose(out);
    fclose(in);
    //encoded file is in "out". Decode
    char *out_decoded = nullptr;
    size_t size_decoded;
    in = fmemopen(buf_out, size_out, "rb");
    FILE *out_dec = open_memstream(&out_decoded, &size_decoded);
    huff.decode_C_stream(in, out_dec);
    fclose(in);
    fclose(out_dec);
    //compare fasta works with C++ streams
    std::stringstream original(str);
    membuf sbuf(out_decoded, out_decoded + size_decoded);
    std::istream decoded(&sbuf);
    return compare_fasta(original, decoded);
}