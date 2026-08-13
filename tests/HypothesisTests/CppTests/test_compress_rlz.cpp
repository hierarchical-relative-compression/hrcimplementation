//
// Created by mpelo on 10/7/25.
//

#include "HRLZTests.h"

#include "test_compress_rlz.hpp"
#include "hrlz/CompressorRLZ.hpp"
#include "hrlz/file.hpp"
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include "rlz/rlz.hpp"
#include "utils_tests.hpp"

int64_t test_rlz_compress_decompress(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta) {
    std::string ref(ref_fasta, n_ref_fasta);
    std::string target(target_fasta, n_target_fasta);
    return test_rlz_compress_decompress(ref, target);
}

int64_t test_rlz_compress_decompress(const std::string& ref_fasta, const std::string& target_fasta) {
    return test_compress_decompress(ref_fasta, target_fasta, true, std::make_unique<Compressors::CompressorRLZ>());
}

/*
int64_t test_rlz_compress_decompress(const std::string& ref_fasta, const std::string& target_fasta) {
    static int id = 1;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::string string_id = std::to_string(id);
    std::string filename_ref = "ref" + string_id + ".fasta";
    std::string filename_ref_sa = filename_ref + ".sa";
    std::string filename_target = "target" + string_id + ".fasta";
    std::string filename_compressed = filename_ref + "_" + filename_target;
    std::string filename_decompressed = "target" + string_id + ".fasta";
    id++;
    std::filesystem::path ref_path = temp_dir / filename_ref;
    std::filesystem::path target_path = temp_dir / filename_target;
    std::filesystem::path ref_sa_path = temp_dir / filename_ref_sa;
    //Create comp dir
    std::filesystem::path comp_dir = temp_dir / "comp_rlz";
    if (!std::filesystem::exists(comp_dir) || !(std::filesystem::is_directory(comp_dir))) {
        std::filesystem::create_directory(comp_dir);
    }
    //Create decomp dir
    std::filesystem::path decomp_dir = temp_dir / "decomp_dir";
    if (!std::filesystem::exists(decomp_dir) || !(std::filesystem::is_directory(decomp_dir))) {
        std::filesystem::create_directory(decomp_dir);
    }
    std::ofstream temp_ref(ref_path, std::ios::out | std::ios::trunc);
    std::ofstream temp_target(target_path, std::ios::out | std::ios::trunc);
    temp_ref.write(ref_fasta.c_str(), ref_fasta.size());
    temp_target.write(target_fasta.c_str(), target_fasta.size());
    //Files are in the disk. Now, we need the SA of the reference
    temp_ref.close();
    temp_target.close();
    int ret = make_SA(ref_path);
    if (ret != 0) {
        std::cerr << "Problem making SA for RLZ fasta" << std::endl;
        return 0;
    }
    //SA has been written. We can compress now
    Compressors::CompressorRLZ *rlz_comp = new Compressors::CompressorRLZ();
    rlz_comp->output_dir = comp_dir;
    rlz_comp->compress_pair_keep(ref_path, target_path, ref_sa_path);
    delete rlz_comp;
    //The compression has been done. Now, decompress
    DynArray<char> ref = read_compacted_DNA(ref_path);
    Compressors::CompressorRLZ::decompress_pair(ref, filename_compressed, filename_target, decomp_dir, comp_dir);
    //And read the output
    std::string out;
    std::ifstream decomp(decomp_dir / filename_decompressed, std::ios::binary);
    std::stringstream buffer;
    buffer << decomp.rdbuf();
    //Destroy the temp files
    std::filesystem::remove(ref_path);
    std::filesystem::remove(ref_sa_path);
    std::filesystem::remove(target_path);
    std::filesystem::remove(comp_dir / filename_compressed);
    std::filesystem::remove(decomp_dir / filename_decompressed);
    //Now, we have to check if the strings are the same
    std::stringstream target_stream(target_fasta);
    int64_t byte_differ = compare_fasta(buffer, target_stream);
    return byte_differ;
}
*/
/*
 * LEGACY
 * try {
        //Call sa.run
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            execlp("sa_fasta.run", "sa_fasta.run", ref_path.c_str(), ref_sa_path.c_str(), (char*)0);
            //If it reaches here, something went wrong
            std::cerr << "The SA for a test fasta couldn't be made" << std::endl;
            return 0; //Error at byte 0
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (status != 0) {
                std::cerr << "SA for a test fasta finished with error code " << status << std::endl;
                return 0; //Error on the first byte
            }

        }
    } catch (std::exception& e) {
        //Destroy the temp files
        std::filesystem::remove(ref_path);
        std::filesystem::remove(ref_sa_path);
        std::filesystem::remove(target_path);
        std::filesystem::remove(comp_dir / filename_compressed);
        std::filesystem::remove(decomp_dir / filename_decompressed);
        return 0;
    }
 */