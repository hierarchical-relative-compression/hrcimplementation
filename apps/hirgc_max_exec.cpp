//
// Created by mpelo on 6/13/24.
//
#include <iostream>
#include <cstring>
#include "hirgc/hirgc_max.hpp"
#include "HuffmanFile.hpp"
#include <filesystem>

using namespace hirgc_mrmcmax;

void old() {
    std::filesystem::path dir("/home/mpelo/Documents/DTU/PHD COMPRESSION/DATA/SARS dataset");
    std::filesystem::path path = dir / "ENA|LR883857|LR883857.1";
    HiRGCRef ref(path, false);
//    ref.compress_target_to_disk(dir / "ENA|FR988026|FR988026.1", dir / "output_hirgc_max", true);
//    ref.decompress_target(dir / "output_hirgc_max" / "ENA|FR988026|FR988026.1", dir / "decomp_max", "ENA|FR988026|FR988026.1");
}

void comp_other_chars() {
    std::ofstream out(std::filesystem::temp_directory_path() / "other_chars.txt", std::ios_base::trunc);
    std::stringstream out_s;
    POSITION_OTHER_CHAR chars[9] = { {1, 'B'}, {2, 'C'}, {3, 'D'},
                                      {4, 'E'}, {5, 'F'}, {6, 'G'},
                                      {7, 'E'}, {8, 'I'}, {9, 'J'}};
    for (int i = 0; i < 9; i++) {
        chars[i].ch = chars[i].ch - 'A';
    }
    hirgc_mrmcmax::print_other_chars(out_s, chars, 9);
    out << out_s.rdbuf();
}

void decomp_other_chars() {
    std::ifstream in(std::filesystem::temp_directory_path() / "other_chars.txt", std::ios_base::in);
    HiRGCTarget *target = new HiRGCTarget();
    target->other_chars = std::vector<POSITION_OTHER_CHAR>();
    std::stringstream ss;
    ss << in.rdbuf();
    read_other_chars(ss, target);
    //Print
    std::size_t n_other_chars = target->other_chars.size();
    POSITION_OTHER_CHAR *other_chars = target->other_chars.data();
    for (std::size_t i = 0; i < n_other_chars; i++) {
        std::cout << other_chars[i].pos << " " << static_cast<u_char>(other_chars[i].ch) << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: hirgcmax_exec -c/-d reference target output_dir [target_original_filename (only if decompressing)]" << std::endl;
        std::exit(1);
    }
    std::filesystem::path reference = argv[2];
    std::filesystem::path target = argv[3];
    std::filesystem::path output_dir = argv[4];
    if (strcmp(argv[1], "-d") == 0) {
        std::string target_original_filename;
        if (argc == 5) {
            target_original_filename = get_filename(reference, target);
        } else if (argc == 6) {
            target_original_filename = argv[5];
        } else {
            std::cerr << "Usage: hirgcmax_exec -c/-d reference target output_dir [target_original_filename (only if decompressing)]" << std::endl;
            std::exit(1);
        }
        std::string target_filename = target.filename().string();
        if (target_filename.ends_with(".fse")) {
            target = target.parent_path() / (target_filename.substr(0, target_filename.size() - 4));
        }
        //Decode
        HuffmanEncoder enc;
        std::filesystem::path decoded_path = std::filesystem::temp_directory_path() / target.filename();
        enc.decode_executable(target, decoded_path);
        //Decompress
        HiRGCRef *ref = new HiRGCRef(reference, true);
        std::ifstream decoded(decoded_path, ref->in_decomp_tar_mode);
        if (!decoded)
            throw std::runtime_error("Coudln't find file " + target.string());
        std::ofstream out(std::filesystem::canonical(output_dir) / target_original_filename, ref->out_decomp_tar_mode);
        if (!out)
            throw std::runtime_error("Couldn't open " + (std::filesystem::canonical(output_dir) / target_original_filename).string() + " to decompress");
        ref->decompress_target(decoded, out);
        decoded.close();
        std::filesystem::remove(decoded_path);
        out.close();
        delete ref;
    } else {
        HiRGCRef *ref = new HiRGCRef(reference, false);
        std::filesystem::path compressed = std::filesystem::temp_directory_path() / get_filename(reference, target);
        std::ofstream output(compressed, ref->out_tar_mode);
        ref->compress_target(target, output);
        output.close();
        HuffmanEncoder enc;
        std::filesystem::path output_encoded = output_dir / get_filename(reference, target);
        enc.encode_executable(compressed, output_encoded);
        std::filesystem::remove(compressed);
        delete ref;
    }
}