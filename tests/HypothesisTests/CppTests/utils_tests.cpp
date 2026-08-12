//
// Created by mpelo on 7/22/24.
//

#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/wait.h>

#include "utils_tests.hpp"
#include "Compressors.hpp"

//Compare fasta
void skip_fasta_line(std::istream& s) {
    char c = s.peek();
    if (c == '>') {
        //We have to ignore the first line
        std::string fasta_line;
        std::getline(s, fasta_line);
    }
}

int64_t compare_fasta(std::istream& stream1, std::istream& stream2) {
    //We skip the first line if it is a FASTA definition line
    skip_fasta_line(stream1);
    skip_fasta_line(stream2);
    //Read char by char, and ignore whitespace and newlines
    int64_t i = 0;
    char c1 = 0;
    char c2 = 0;
    while (stream1 && stream2) {
        if (c1 != c2) {
            return i-1;
        }
        i++;
        stream1 >> c1; //skipping whitespace
        stream2 >> c2;
    }
    if (!stream1) {
        if (!stream2) {
            //All good
            i = -1;
            return i;
        } else {
            return i;
        }
    } else {
        //File 1 didn't end but file 2 did!
        return i;
    }
}

//Serialize targets
int serialize_two(const std::string& ref, const std::string& target,
               std::filesystem::path& ref_path, std::filesystem::path& target_path) {
    //First, serialize the reference and the target.
    static int id = 1;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::string string_id = std::to_string(id);
    std::string filename_ref = "ref" + string_id + ".fasta";
    std::string filename_target = "target" + string_id + ".fasta";
    id++;
    ref_path = temp_dir / filename_ref;
    target_path = temp_dir / filename_target;
    try {
        std::ofstream temp_ref(ref_path, std::ios::out | std::ios::trunc);
        std::ofstream temp_target(target_path, std::ios::out | std::ios::trunc);
        temp_ref.write(ref.c_str(), ref.size());
        temp_target.write(target.c_str(), target.size());
        temp_ref.close();
        temp_target.close();
        return 0;
    } catch (std::ios_base::failure& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}

int make_SA(const std::filesystem::path& fasta) {
    if (!std::filesystem::exists(fasta)) {
        std::cerr << "File " << fasta.string() << " does not exist! Cannot create FASTA!" << std::endl;
        return -1;
    }
    std::string target_str = fasta.string();
    std::string target_sa = target_str + ".sa";
    std::filesystem::path sa_fasta = PATH_SA_FASTA;
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        execl(sa_fasta.c_str(), "sa_fasta", target_str.c_str(), target_sa.c_str(), (char*)0);
        //If it reaches here, something went wrong
        std::cerr << "The SA for a test fasta couldn't be made" << std::endl;
        return -1;
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            std::cerr << "SA for a test fasta finished with error code " << status << std::endl;
            return status;
        }
        return 0;
    }
}

int64_t test_compress_decompress(const std::string& ref_fasta, const std::string& target_fasta, bool SA, std::unique_ptr<Compressors::Compressor> comp) {
    //First, serialize the reference and the target.
    std::filesystem::path ref_path, target_path;
    serialize_two(ref_fasta, target_fasta, ref_path, target_path);
    std::filesystem::path temp_dir = ref_path.parent_path();
    if (SA) {
        int ret = make_SA(ref_path);
        if (ret != 0) {
            //SA_FASTA had a problem. Return fail on byte 0
            return 0;
        }
    }
    //Files are in disk. Compress and decompress.
    try {
        const std::filesystem::path **targets = new const std::filesystem::path*[1];
        targets[0] = &target_path;
        const std::string& tar_name = target_path.filename();
        const std::string& ref_name = ref_path.filename();
        //Compress
        comp->compress_one_to_many(ref_path, targets, 1, temp_dir);
        std::filesystem::path encoded = comp->get_encoded_file(ref_name, tar_name,
                                                              std::filesystem::canonical(temp_dir));
        if (!std::filesystem::exists(encoded)) {
            std::cerr << "Compressed file with " << comp->type << " does not exist!" << std::endl;
            return 0;
        }
        //Decompress
        std::string subdir = "decomp_" + comp->type;
        //We want to make a subdirectory for the decompressed files
        if (!std::filesystem::exists(temp_dir / subdir)) {
            std::filesystem::create_directory(temp_dir / subdir);
        }
        std::filesystem::path *target_compressed = new std::filesystem::path[1];
        const std::string **tar_original_names = new const std::string*[1];
        target_compressed[0] = comp->get_output_file(ref_name, tar_name, std::filesystem::canonical(temp_dir)); //The targets are in input_dir
        tar_original_names[0] = &tar_name;
        comp->decompress_one_to_many(ref_path, target_compressed, tar_original_names, 1, temp_dir / subdir);
        std::filesystem::path decompressed_target = temp_dir / subdir / tar_name;
        if (!std::filesystem::exists(decompressed_target)) {
            std::cerr << "Decompressed file with " << comp->type << " does not exist!" << std::endl;
            return 0;
        }
        //Read the target
        std::ifstream decomp(decompressed_target, std::ios::binary);
        std::stringstream buffer;
        buffer << decomp.rdbuf();
        //Check if the strings are the same
        std::stringstream target_stream(target_fasta);
        int64_t byte_differ = compare_fasta(buffer, target_stream);
        //Time to remove all files
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        std::filesystem::remove(encoded);
        std::filesystem::remove(decompressed_target);
        if (SA) {
            std::filesystem::remove(ref_path.string() + ".sa");
        }
        delete[] target_compressed;
        delete[] tar_original_names;
        delete[] targets;
        return byte_differ;
    } catch (std::exception& e) {
        std::cerr << "Error while compressing with " << comp->type << std::endl;
        std::cerr << e.what() << std::endl;
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        return 0;
    }
}

//SufCheck

int
sufcheck(const unsigned char *T, const unsigned int *SA, unsigned int n, int verbose) {
    unsigned int C[256];
    unsigned int i, p, q, t;
    unsigned int c;

    if(verbose) { fprintf(stderr, "sufcheck: "); }
    if(n == 0) {
        if(verbose) { fprintf(stderr, "Done.\n"); }
        return 0;
    }

    /* Check arguments. */
    if((T == NULL) || (SA == NULL)) {
        if(verbose) { fprintf(stderr, "Invalid arguments.\n"); }
        return -1;
    }

    /* check range: [0..n-1] */
    for(i = 0; i < n; ++i) {
        if(n <= SA[i]) {
            if(verbose) {
                fprintf(stderr, "Out of the range [0,%d].\n"
                                "  SA[%d]=%d\n",
                                n - 1, i, SA[i]);
            }
            return -2;
        }
    }

    /* check first characters. */
    for(i = 1; i < n; ++i) {
        if(T[SA[i - 1]] > T[SA[i]]) {
            if(verbose) {
                fprintf(stderr, "Suffixes in wrong order.\n"
                                "  T[SA[%d]=%d]=%d > T[SA[%d]=%d]=%d\n",
                                i - 1, SA[i - 1], T[SA[i - 1]], i, SA[i], T[SA[i]]);
            }
            return -3;
        }
    }

    /* check suffixes. */
    for(i = 0; i < 256; ++i) { C[i] = 0; }
    for(i = 0; i < n; ++i) { ++C[T[i]]; }
    for(i = 0, p = 0; i < 256; ++i) {
        t = C[i];
        C[i] = p;
        p += t;
    }

    q = C[T[n - 1]];
    C[T[n - 1]] += 1;
    for(i = 0; i < n; ++i) {
        p = SA[i];
        if(0 < p) {
            c = T[--p];
            t = C[c];
        } else {
            c = T[p = n - 1];
            t = q;
        }
        if(p != SA[t]) {
            if(verbose) {
                fprintf(stderr, "Suffix in wrong position.\n"
                                "  SA[%d]=%d or\n"
                                "  SA[%d]=%d\n",
                                t, SA[t], i, SA[i]);
            }
            return -4;
        }
        if(t != q) {
            ++C[c];
            if((n <= C[c]) || (T[SA[C[c]]] != c)) { C[c] = -1; }
        }
    }

    if(1 <= verbose) { fprintf(stderr, "Done.\n"); }
    return 0;
}

