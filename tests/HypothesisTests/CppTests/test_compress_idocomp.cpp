//
// Created by mpelo on 7/18/24.
//

#include "HRLZTests.h"

#include "iDoComp/iDoCompInterface.hpp"
#include <filesystem>
#include <string>
#include <fstream>

//POSIX API
//Part of the OS libraries and not C, so we need to put .h
#include <unistd.h>
#include <sys/wait.h>
//To send and handle signals to processes
#include <sys/prctl.h>
#include <signal.h>
#include <iostream>

#include "utils_tests.hpp"
#include "test_compress_idocomp.hpp"

int64_t test_idocomp_compress_decompress(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta) {
    std::string ref(ref_fasta, n_ref_fasta);
    std::string target(target_fasta, n_target_fasta);
    return test_idocomp_compress_decompress(ref, target);
}

void remove_compressed(std::string& filename) {
    std::filesystem::path temp = std::filesystem::temp_directory_path();
    std::string char_ido_filename = filename + "_char.ido";
    std::string headers_ido_filename = filename + "_headers.ido";
    std::string ints_ido_filename = filename + "_ints.ido";
    std::string signs_ido_filename = filename + "_signs.ido";
    std::filesystem::remove(temp / char_ido_filename);
    std::filesystem::remove(temp / headers_ido_filename);
    std::filesystem::remove(temp / ints_ido_filename);
    std::filesystem::remove(temp / signs_ido_filename);
}

//Functions to test a specific fasta file

int64_t test_idocomp_compress_decompress(const std::string& ref_fasta, const std::string& target_fasta) {
    //First, we need to write the string to a fasta file.
    static int id = 1;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::string string_id = std::to_string(id);
    std::string filename_ref = "ref" + string_id + ".fasta";
    std::string filename_ref_sa = filename_ref + ".sa";
    std::string filename_target = "target" + string_id + ".fasta";
    std::string filename_compressed = "comp_target" + string_id;
    std::string filename_decompressed = "decomp_target" + string_id;
    id++;
    std::filesystem::path ref_path = temp_dir / filename_ref;
    std::filesystem::path target_path = temp_dir / filename_target;
    std::filesystem::path ref_sa_path = temp_dir / filename_ref_sa;
    std::filesystem::path compressed_path = temp_dir / filename_compressed;
    std::ofstream temp_ref(ref_path, std::ios::out | std::ios::trunc);
    std::ofstream temp_target(target_path, std::ios::out | std::ios::trunc);
    temp_ref.write(ref_fasta.c_str(), ref_fasta.size());
    temp_target.write(target_fasta.c_str(), target_fasta.size());
    //Files are in the disk. Now, we need the SA of the reference
    temp_ref.close();
    temp_target.close();
    try {
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
            //SA has been written. We can compress now
            iDoComp_compress(ref_path, ref_sa_path, target_path, compressed_path);
            //The compression has been done. Now, decompress
            iDoComp_decompress(ref_path, filename_decompressed, compressed_path, temp_dir);
            //And read the output
            std::string out;
            std::ifstream decomp(temp_dir / filename_decompressed, std::ios::binary);
            std::stringstream buffer;
            buffer << decomp.rdbuf();
            //Destroy the temp files
            std::filesystem::remove(ref_path);
            std::filesystem::remove(ref_sa_path);
            std::filesystem::remove(target_path);
            remove_compressed(filename_compressed);
            std::filesystem::remove(temp_dir / filename_decompressed);
            //Now, we have to check if the strings are the same
            std::stringstream target_stream(target_fasta);
            int64_t byte_differ = compare_fasta(buffer, target_stream);
            return byte_differ;
        }
    } catch (std::exception& e) {
        //Destroy the temp files
        std::filesystem::remove(ref_path);
        std::filesystem::remove(ref_sa_path);
        std::filesystem::remove(target_path);
        remove_compressed(filename_compressed);
        std::filesystem::remove(temp_dir / filename_decompressed);
        return 0;
    }
}

