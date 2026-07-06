#include "hrlz/file.hpp"
#include "utils/PrettyPrint.hpp"

#include <iostream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <cstdint>

void zip(const std::filesystem::path& path, const std::filesystem::path& output_dir) {
    std::ostringstream oss;
    std::filesystem::path zip_path = output_dir / path.filename();
    const char *target_str = path.c_str();
    //const char *zip_str = zip_path.c_str();
    oss << "7za a " << zip_path << ".7z \"" << target_str << "\" -m0=PPMd";
    std::string cmd = oss.str();
    std::cout << "Calling 7za with " << cmd << std::endl;
    system(cmd.c_str());
    oss.clear();
    //auto p = std::chrono::steady_clock::duration::zero();
}
/*
    std::stringstream os;
    std::vector<std::string> headers{"Dataset", "Method", "Compressed size", "Compression time"};
    std::vector<alignment> alignments {LEFT, LEFT, RIGHT, RIGHT};
    print_header(os, headers, alignments);
    print_row(os, std::vector<std::string> {"Hey", "Joe", format_size(127000034), format_size(80023)});
    print_row(os, std::vector<std::string> {format_time(9000000000), format_time(600000), format_time(800000000), format_time(109485274182)});
    std::cout << os.rdbuf() << std::endl;
*/

void gang(int64_t (*yap)(int, int)) {
    std::cout << yap(3, 7);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    std::string test;
    uint64_t x = 7ULL;
    test += x;
    std::cout << test << std::endl;
}