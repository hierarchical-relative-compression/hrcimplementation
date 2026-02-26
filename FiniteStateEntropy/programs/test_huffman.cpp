//
// Created by mpelo on 12/19/25.
//
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#include "huf.h"
#include "HuffmanFile.hpp"

int main(int argc, char *argv[]) {
    std::string path = "/home/mpelo/DATA_COMPRESSION/Chromosome19/my_comp";
    std::filesystem::path file(path);
    std::filesystem::path output_file(path + "_comp");
    std::filesystem::path decomp_file(path + "_dec");
    //Read file
    std::ifstream large(file, std::ios_base::binary | std::ios_base::in);
    if (!large)
        throw std::runtime_error("Couldn't open " + file.string());
    size_t text_size = std::filesystem::file_size(file);
    std::string my_text(text_size, '\0');
    if (!large.read(&my_text[0], text_size))
        throw std::runtime_error("Coudn't read: " + file.string());
    large.close();
    //Compress
    size_t comp_size = huffman_compress(my_text, output_file);
    std::cout << "Compressed in " << comp_size << " bytes" << std::endl;
    //now, decompress
    huffman_decompress(output_file, decomp_file);
    std::cout << "Decompressed" << std::endl;
    //Read decompressed file
    std::ifstream dec(decomp_file, std::ios_base::binary | std::ios_base::in);
    if (!dec)
        throw std::runtime_error("Couldn't open decompressed file " + decomp_file.string());
    char *dec_buf = new char[text_size];
    if (!dec.read(dec_buf, text_size)) {
        delete[] dec_buf;
        throw std::runtime_error("Couldn't read from " + decomp_file.string());
    }
    //Check
    for (size_t i = 0; i < text_size; i++) {
        if (!my_text[i] == dec_buf[i]) {
            return i;
        }
    }
    delete[] dec_buf;
    return -1;
}

void only_small() {
    std::string my_text = "Voy al transbordador (bam bam) a descargar la carreta (bam bam)";
    size_t text_size = my_text.size();
    size_t dest_size = HUF_compressBound(text_size);
    char *dest = new char[dest_size];
    size_t comp_size = HUF_compress(dest, dest_size, my_text.c_str(), text_size);
    std::cout << "Compressed: " << std::endl;
    std::cout << dest << std::endl;
    std::cout << "Original text: " << text_size << " Bytes, compressed size: " << comp_size << " Bytes" << std::endl;
    //now, decompress
    char *decompressed = new char[dest_size];
    size_t decomp_size = HUF_decompress(decompressed, text_size, dest, comp_size);
    std::cout << "Decompressed: " << std::endl;
    std::cout << decompressed << std::endl;
    std::cout << "Size of decompressed: " << decomp_size << std::endl;
    delete[] dest;
    delete[] decompressed;
}