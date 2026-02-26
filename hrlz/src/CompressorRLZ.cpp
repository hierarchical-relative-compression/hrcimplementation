//
// Created by mpelo on 10/6/25.
//

#include <fstream>
#include "hrlz/Compressors.hpp"
#include "hrlz/CompressorRLZ.hpp"
#include "hrlz/file.hpp"
#include <queue>
#include "common/SteadyTimer.hpp"
#include "ShuffEncoder.hpp"
#include <omp.h>

namespace Compressors {

    std::string CompressorRLZ::get_filename(const std::filesystem::path &ref, const std::filesystem::path &target) {
        return ref.filename().string() + "_" + target.filename().string();
    }

    CompressorRLZ::CompressorRLZ() : Compressor(new ShuffEncoder()) {
        rlz_compressor.reset(nullptr);
    }

    CompressorRLZ::~CompressorRLZ() {
        rlz_compressor.reset(nullptr);
        delete encoder;
    }

    void zip_target(const std::filesystem::path& file, const std::filesystem::path& archive_zip) {
        std::ostringstream oss;
        //The canonical path is needed because of a weird bug: if the path was serialized as "folder/file.fa" instead of "./folder/file.fa",
        //then 7za would take "folder/file.fa" as the file name and create a subdirectory in the 7z file, thus messing up with the
        //folder structure later on.
        oss << DIR_7ZA << "/7za a \"" << archive_zip.string() << "\" \"" << std::filesystem::canonical(file).c_str() << "\" -m0=PPMd" << " >/dev/null";
        std::string cmd = oss.str();
        //std::cout << "Calling 7za with " << cmd << std::endl;
        system(cmd.c_str());
        oss.clear();
        //Check that the zip file is there
        if (std::filesystem::exists(archive_zip)) {
            std::filesystem::remove(file);
        } else {
            std::cerr << "Zip file " << archive_zip << " does not exist" << std::endl;
            std::exit(1);
        }
    }

    void unzip_target(const std::filesystem::path& target, const std::filesystem::path& output_dir) {
        std::string cmd = "7za x \"";
        cmd.append(target.string()).append("\" -o\"").append(std::filesystem::canonical(output_dir).string()).append("\"").append(" >/dev/null");
#ifndef NDEBUG
        std::cout << "Calling 7za with " << cmd << std::endl;
#endif
        system(cmd.c_str());
    }

    /**
     * The output file must contain the extension .huf
     * @param file full path to the input file.
     * @param output_file full path to the output file
     */
    void fse_encode_target(const std::filesystem::path& file, const std::filesystem::path& output_file) {
        std::string cmd = DIR_FSE;
        cmd.append("/fse_exec -h \"").append(std::filesystem::canonical(file)).append("\" \"").append(output_file.string()).append("\" >/dev/null 2>/dev/null");
        int ret = system(cmd.c_str());
        if (ret != 0) {
            std::cerr << "WARNING: Call to fse returned exit code " << ret << std::endl;
        }
        if (!std::filesystem::exists(output_file)) {
            throw std::runtime_error("Fse didn't produce output file " + output_file.string() + "!!!");
        }
        std::filesystem::remove(file);
    }

    /**
     * The input file must have the extension .huf
     * @param file full path to compressed file with extension .huf
     * @param output_file full path to the future decompressed file
     */
    void fse_decode_target(const std::filesystem::path& file, const std::filesystem::path& output_file) {
        std::string cmd = DIR_FSE;
        cmd.append("/fse_exec -d \"").append(std::filesystem::canonical(file)).append("\" \"").append(output_file.string()).append("\" >/dev/null 2>/dev/null");
        int ret = system(cmd.c_str());
        if (ret != 0) {
            std::cerr << "WARNING: Call to fse returned exit code " << ret << std::endl;
        }
    }

    /**
     * Methods inherited from Compressor
    */
    size_t CompressorRLZ::compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                const std::filesystem::path& ref_sa) {
        std::size_t weight = compress_pair_keep(ref, file_to_parse, ref_sa);
        std::string last_filename;
        switch (encoding_type) {
            case ARITH:
                last_filename = get_filename(ref, file_to_parse);
                break;
            case PPM:
                last_filename = get_filename(ref, file_to_parse) + ".7z";
                break;
            case HUFF:
                last_filename = get_filename(ref, file_to_parse) + ".huf";
                break;
        }
        std::filesystem::remove(output_dir / last_filename);
        return weight;
    }
    size_t CompressorRLZ::compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                              const std::filesystem::path& ref_sa) {
        std::string name = ref.filename().string();
        try {
            if (rlz_compressor.get() == nullptr) {
                rlz_compressor = RLZCompress::from_dynamic_fasta(ref, ref_sa);
                rlz_compressor->set_id(name);
            } else if (rlz_compressor->get_id() != name) {
                rlz_compressor = RLZCompress::from_dynamic_fasta(ref, ref_sa);
                rlz_compressor->set_id(name);
            }
            //Create output file
            std::string out_filename = get_filename(ref, file_to_parse);
            std::ofstream out_file(output_dir / out_filename, std::ios_base::binary | std::ios_base::out);
            if (!out_file) {
                std::cerr << "Couldn't open file " << output_dir / out_filename << "!!!" << std::endl;
                throw std::runtime_error("IOError");
            }
            FactorStreamWriter writer(&out_file);
            //Compress it with RLZ
            rlz_compressor->compress_fasta(file_to_parse.c_str(), writer);
            out_file.flush();
            out_file.close(); //Important to flush
            //encode it
            std::string last_filename;
            switch (encoding_type) {
                case ARITH:
                    throw std::runtime_error("Arith not implemented for RLZ");
                case PPM:
                {
                    last_filename = out_filename + ".7z";
                    zip_target(output_dir / out_filename, output_dir / last_filename);
                    break;
                }
                case HUFF:
                    last_filename = out_filename + ".huf";
                    fse_encode_target(output_dir / out_filename, output_dir / last_filename);
                    break;
            }
            //Return size
            std::size_t weight = std::filesystem::file_size(output_dir / last_filename);
            return weight;
        } catch (std::exception& e) {
            std::cerr << "Couldn't compress " << file_to_parse << "w.r.t. " << ref << std::endl;
            std::cerr << e.what();
            throw e;
        }
    }

    void CompressorRLZ::decompress_pair(const DynArray<char>& seq_ref, const std::string& tar_comp_filename,
                         const std::string& tar_original_filename,
                         const std::filesystem::path& output_dir, const std::filesystem::path& input_directory) {
        //Decode
        switch (encoding_type) {
            case ARITH:
                throw new std::runtime_error("Arith not implemented for RLZ");
            case PPM:
                ProfilerSteady::profile_thread("decode_target_" + std::to_string(omp_get_thread_num()), [&] {
                    unzip_target(input_directory / (tar_comp_filename + ".7z"), output_dir);
                }, false);
                break;
            case HUFF:
                ProfilerSteady::profile_thread("decode_target_" + std::to_string(omp_get_thread_num()), [&] {
                    fse_decode_target(input_directory / (tar_comp_filename + ".huf"), output_dir / tar_comp_filename);
                }, false);
                break;
        }
        //Decompress pair
        ProfilerSteady::profile_thread("decompress_target_" + std::to_string(omp_get_thread_num()), [&] {
            std::ifstream comp_tar(output_dir / tar_comp_filename, std::ios_base::binary);
            std::ofstream decomp_tar(output_dir / tar_original_filename, std::ios_base::binary | std::ios_base::out);
            if (comp_tar.peek() == '>') {
                std::string line;
                std::getline(comp_tar, line);
                decomp_tar.write(line.c_str(), line.size());
                decomp_tar.write("\n", 1);
            }
            uint64_t pos, len;
            comp_tar.read((char *)&pos, sizeof(pos));
            while (comp_tar) {
                comp_tar.read((char *)&len, sizeof(len));
                if (len == 0) {
                    decomp_tar.put(pos);
                } else {
                    decomp_tar.write(seq_ref.begin() + pos, len);
                }
                //Prepare for next iteration
                comp_tar.read((char *)&pos, sizeof(pos));
            }
            comp_tar.close();
            //Remove leftover file
            std::filesystem::remove(output_dir / tar_comp_filename);
        }, false);
    }//both comp and decomp streams close

    void CompressorRLZ::decompress_pair(const std::filesystem::path& ref, const std::filesystem::path& target, const std::filesystem::path& output) {
        DynArray<char> ref_seq;
        ProfilerSteady::profile_thread("load_ref_" + std::to_string(omp_get_thread_num()), [&] {
            ref_seq = read_compacted_DNA(ref);
        }, false);
        std::string target_comp = get_filename(ref, target);
        decompress_pair(ref_seq, target_comp, output.filename(), output.parent_path(), target.parent_path());
    }

    std::unique_ptr<RefDataStructure> CompressorRLZ::getRefDataStructure(const std::filesystem::path& ref, bool decomp) {
        return std::make_unique<RLZCompress>(ref, decomp);
    }

    std::unique_ptr<RefDataStructure> CompressorRLZ::getRefDataStructure(std::istream& ref, bool decomp) {
        return std::make_unique<RLZCompress>(ref, decomp);
    }

}