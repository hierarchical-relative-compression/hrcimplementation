#include<access/hrlzFile.hpp>
#include<fstream>
#include<iostream>

int main(int argc, char** argv) {
  (void)argc;
  std::filesystem::path compressed_file(argv[1]);
  std::size_t file_index = std::stoi(argv[2]);
  std::size_t index = std::stoi(argv[3]);

  std::ifstream compressed_file_stream(compressed_file);
  HRLZFile hrlzfile = construct_hrlzfile(compressed_file_stream);
  std::cout << hrlzfile.file_path(file_index) << std::endl;
  std::cout << hrlzfile.find(file_index, index) << std::endl;
}
