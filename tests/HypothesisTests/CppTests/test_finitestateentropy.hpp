//
// Created by mpelo on 12/19/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_TEST_FINITESTATEENTROPY_HPP
#define HIERARCHICALRELATIVELEMPELZIV_TEST_FINITESTATEENTROPY_HPP

#include <string>
int64_t compress_decompress_huffman(const std::string& str);

int64_t compress_decompress_huffman_api(const std::string& str);

int64_t compress_decompress_fse_mem(const std::string& str);

#endif //HIERARCHICALRELATIVELEMPELZIV_TEST_FINITESTATEENTROPY_HPP
