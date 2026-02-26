//
// Created by mpelo on 4/11/24.
//
#include "SCCGLib/testPipe.hpp"
#include "hrlz/CompressorSCCG.hpp"
#include <queue>
#include <filesystem>
#include <HuffmanFile.hpp>
#include <iostream>

namespace Compressors {

    /**
     * Methods to handle the connection to the Java process
     */
    bool java_open = false;

    void CompressorSCCG::check_java() {
        if (!java_open) {
	    std::cout << "Opening java at: " << std::filesystem::current_path() << std::endl;
            MrMCMax::open_java();
            java_open=true;
        }
    }
    void CompressorSCCG::end_java() {
        MrMCMax::end_java();
        java_open=false;
    }

    /**
     * METHODS CompressorSCCG
     */

    CompressorSCCG::CompressorSCCG() : Compressor(new HuffmanEncoder()) {
        check_java();
    }
    CompressorSCCG::~CompressorSCCG() {
        delete encoder;
    }

    size_t CompressorSCCG::compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                const std::filesystem::path& ref_sa) {
        return compress_pair_delete(ref, file_to_parse);
    }
    size_t CompressorSCCG::compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                              const std::filesystem::path& ref_sa) {
        return compress_pair_keep(ref, file_to_parse);
    }

    size_t CompressorSCCG::compress_dataset(const DynArray<std::filesystem::path>& files,
                                            const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id,
                                            const DynArray<std::filesystem::path>& sa_files) {
        return compress_hrlz(files, arbor, root_id);
    }


    size_t CompressorSCCG::compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse) {
        //Get the invocation line "ref target output_folder/filename" in a non-const C string
        std::string output_folder = output_dir.string();
        std::string line = "c \"" + ref.string() + "\"" + " " + "\"" + file_to_parse.string() + "\"" + " " + + "\"" + output_folder + "\"\n";
        std::size_t size = line.size();
        char *invocation = new char[size + 1];
        line.copy(invocation, size);
        invocation[size] = '\0';
        MrMCMax::write_line_print_output(invocation);
        //Read file size and return
        std::string output_file = (output_folder / file_to_parse.filename()).string() + ".7z";
        if (!std::filesystem::exists(output_file)) {
            //There has been an error in SCCG
            std::cerr << "Error in SCCGC: the file " << output_file << " was not created" << std::endl;
            std::exit(1);
        }
        std::size_t weight = filesystem::file_size(output_file);
        return weight;
    }

    size_t CompressorSCCG::compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse) {
        std::size_t weight = compress_pair_keep(ref, file_to_parse);
        std::filesystem::path output = (output_dir / file_to_parse.filename()).string() + ".7z";
        if (!std::filesystem::remove(output)) {
            std::cerr << "Attempted to remove " << output << " but wasn't found" << std::endl;
        }
        return weight;
    }

    size_t CompressorSCCG::compress_hrlz(const DynArray<std::filesystem::path>& files,
                       const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id) {
        size_t weight = 0;
        //Standard BFS Traversal
        std::queue<std::size_t> queue;
        std::vector<std::size_t> *adj = arbor.get();
        queue.push(root_id);
        while (!queue.empty()) {
            std::size_t ref = queue.front();
            queue.pop();
            std::vector<std::size_t> targets = adj[ref];
            for (std::size_t j = 0; j < targets.size(); j++) {
                std::size_t target_id = targets[j];
                weight += compress_pair_keep(files[ref], files[target_id]);
                queue.push(target_id);
            }
        }
        //Hierarchy is compressed. Write the root and the metadata
        if (std::filesystem::exists(output_dir / (files[root_id].filename()))) {
            std::cerr << "Warning: root already exists at " << output_dir / (files[root_id].filename()) << std::endl;
        } else {
            std::filesystem::copy(files[root_id], output_dir / (files[root_id].filename()));
        }
        write_external_metadata(files, arbor, output_dir, root_id);
        return weight;
    }

    void CompressorSCCG::decompress_pair(const std::filesystem::path& ref_filename, const std::filesystem::path& target_filename) {
        //Get the invocation line "ref target output_folder/filename" in a non-const C string
        std::string output_folder = output_dir.string();
        std::string line = "d \"" + ref_filename.string() + "\"" + " " + "\"" + target_filename.string() + "\"" + " " + + "\"" + output_folder + "\"\n";
        std::size_t size = line.size();
        char *invocation = new char[size + 1];
        line.copy(invocation, size);
        invocation[size] = '\0';
        MrMCMax::write_line_print_output(invocation);
    }

    void CompressorSCCG::decompress_dataset(const std::filesystem::path& input_directory) {
        std::unique_ptr<ExternalMetadata> met = read_external_metadata(input_directory);
        std::size_t root = met->root_id;
        std::string *filenames = met->filenames.get();
        std::string root_filename =filenames[root];
        std::vector<std::size_t> *adjs = met->arbor.get();
        //Copy the root to the output folder
        if (std::filesystem::exists(output_dir / root_filename)) {
            std::cerr << "Warning: root already exists in " << output_dir / root_filename << std::endl;
        } else {
            std::filesystem::copy(input_directory / root_filename, output_dir / root_filename);
        }
        //Now, all references will be in the output folder, while all the targets will be in the input folder
        //BFS traversal
        std::queue<std::size_t> queue;
        queue.push(root);
        while (!queue.empty()) {
            std::size_t ref = queue.front();
            queue.pop();
            std::vector<std::size_t> adj = adjs[ref];
            std::string ref_path = output_dir / filenames[ref];
            for (std::size_t j = 0; j < adj.size(); j++) {
                std::size_t target = adj[j];
                queue.push(target);
                std::string target_path = input_directory / (filenames[target] + ".7z");
                decompress_pair(ref_path, target_path);
            }
        }
    }



}
