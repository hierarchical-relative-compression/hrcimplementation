//
// Created by mpelo on 12/20/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_HUFFMANFILE_HPP
#define HIERARCHICALRELATIVELEMPELZIV_HUFFMANFILE_HPP

#include <filesystem>
#include <iostream>
#include <string>
#include "Encoder.hpp"

size_t huffman_compress(const std::string& str, const std::filesystem::path& output_file);
void huffman_decompress(const std::filesystem::path& compressed_file, const std::filesystem::path& output);

class HuffmanEncoder : public Encoder {
private:
    bool idocomp {false};
public:
    HuffmanEncoder();
    ~HuffmanEncoder();
    void set_idocomp() {
        idocomp = true;
    }
    void decode_executable(const std::filesystem::path& in_path, const std::filesystem::path& out_path) const override;
    void encode_executable(const std::filesystem::path& in_path, const std::filesystem::path& out_path) const override;

    void encode_C_stream(FILE* in_stream, FILE* out_stream) const override;
    void decode_C_stream(FILE* in_path, FILE* out_stream) const override;
};

#endif //HIERARCHICALRELATIVELEMPELZIV_HUFFMANFILE_HPP
