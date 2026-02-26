//
// Created by mpelo on 3/11/25.
//
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstddef>
#include <cstdint>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: Array64ToArray32 array64file array32file" << std::endl;
    }
    std::filesystem::path array64path = argv[1];
    std::filesystem::path array32path = argv[2];
    std::ifstream array64file(array64path, std::ios_base::binary);
    if (!array64file) {
        std::cerr << "Couldn't open " << array64path << std::endl;
        return 1;
    }
    array64file.seekg(0, array64file.end);
    size_t length = array64file.tellg();
    array64file.seekg(0);
    size_t n_longs = length >> 3;
    uint64_t *arr = new uint64_t[n_longs];
    array64file.read(reinterpret_cast<char*>(arr), n_longs << 3);
    uint32_t *new_arr = new uint32_t[n_longs];
    for (size_t i = 0; i < n_longs; i++) {
        if (arr[i] > static_cast<uint64_t>(UINT32_MAX)) {
            std::cerr << "Larger than 32 bits int: " << arr[i] << " at position " << i << std::endl;
            delete[] new_arr;
            delete[] arr;
            return 1;
        }
        new_arr[i] = static_cast<uint32_t>(arr[i]);
    }
    delete[] arr;
    array64file.close();
    std::ofstream array32file(array32path, std::ios_base::binary);
    array32file.write(reinterpret_cast<char*>(new_arr), n_longs << 2);
    delete[] new_arr;
    array32file.close();
    return 0;
}