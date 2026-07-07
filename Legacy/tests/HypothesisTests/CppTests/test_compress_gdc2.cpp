//
// Created by mpelo on 7/23/24.
//

#include "test_compress_gdc2.hpp"
#include "../../../hrlz/include/hrlz/CompressorGDC2.hpp"
#include "utils_tests.hpp"
#include <filesystem>
#include <fstream>
#include "hrlz/file.hpp"

int64_t test_compress_gdc2(const std::string& ref_fasta, const std::string& target_fasta) {
    //Serialize ref and target
    static int id = 1;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::string string_id = std::to_string(id);
    std::string filename_ref = "ref" + string_id + ".fasta";
    std::string filename_target = "target" + string_id + ".fasta";
    id++;
    std::filesystem::path ref_path = temp_dir / filename_ref;
    std::filesystem::path target_path = temp_dir / filename_target;
    try {
        std::ofstream temp_ref(ref_path, std::ios::out | std::ios::trunc);
        std::ofstream temp_target(target_path, std::ios::out | std::ios::trunc);
        temp_ref.write(ref_fasta.c_str(), ref_fasta.size());
        temp_target.write(target_fasta.c_str(), target_fasta.size());
        temp_ref.close();
        temp_target.close();
    } catch (std::ios_base::failure& e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    //The files are in the disk. Compress
    std::filesystem::path comp_dir = temp_dir / "comp_gdc2";
    if (!std::filesystem::exists(comp_dir))
        std::filesystem::create_directory(comp_dir);
    try {
        Compressors::CompressorGDC2 *comp = new Compressors::CompressorGDC2();
        comp->output_dir = comp_dir;
        std::string target_str = target_path.string();
        std::string *target_ptr = &target_str;
        std::string *target_pack[1] = {target_ptr};
        comp->compress_sequence_and_keep(ref_path, target_pack, 1);
        //Compressed with the archive name being the reference only. Decompress
        comp->decompress_sequence(filename_ref, comp_dir);
        //Leaves the files in the original directory (temp_dir) with the extension .ori
        std::filesystem::path decomp_path = temp_dir / (filename_target + ".ori");
        if (!std::filesystem::exists(decomp_path)) {
            std::cerr << "No decompression found!" << std::endl;
            return 0;
        }
        std::ifstream decomp(decomp_path, std::ios::binary);
        std::stringstream buffer;
        buffer << decomp.rdbuf();
        //Check if the strings are the same
        std::stringstream target_stream(target_fasta);
        int64_t byte_differ = compare_fasta(buffer, target_stream);
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        comp->gdc2_delete_file(comp_dir / filename_ref);
        std::filesystem::remove(decomp_path);
        std::filesystem::remove(temp_dir / (filename_ref + ".ori"));
        delete comp;
        return byte_differ;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        return 0;
    }
}