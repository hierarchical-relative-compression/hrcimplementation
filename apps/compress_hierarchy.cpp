//
// Created by max on 4/3/24.
//
#include <iostream>
#include <cstring>
#include "hrlz/file.hpp"
#include "hrlz/Compressors.hpp"
#include "common/Timer.hpp"
#include "common/SteadyTimer.hpp"
#include <queue>
#include <fstream>

//Max: last parameters
size_t total_size = 0;
size_t root_size = 0;
std::vector<std::string> chr_names;
std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> start;

void compress_hierarchy(const std::filesystem::path& met_path, const std::filesystem::path& out_folder, const std::string& metadata_name="metadata");

int main(int argc, char *argv[]) {
    start = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t(start);
    std::cout << "Started the computation at " << std::ctime(&start_time) << std::endl;
    if (argc < 4) {
        std::cout << "Usage: " << std::endl;
        std::cout
                << "compress_hierarchy /path/to/metadata_dir/ -o /path/to/output_folder [--compressor comp] [--chromosomes chr_name_list.txt]"
                << std::endl;
        std::exit(0);
    }
    std::filesystem::path met_path = argv[1];
    std::filesystem::path out_folder = argv[3];
    filesystem::init_output_folder(out_folder);
    if (argc >= 6) {
        if (strcmp(argv[4], "--compressor") != 0) {
            std::cerr << "Unknown command " << argv[4] << std::endl;
            std::exit(1);
        }
        std::string selected_compressor = argv[5];
        //Seems that C++ doesn't support switch statements on strings.
        if (selected_compressor == "rlz") Compressors::compressor_type = Compressors::RLZ;
        else if (selected_compressor == "hirgcmax") Compressors::compressor_type = Compressors::HIRGCMAX;
        else if (selected_compressor == "idocompmax") Compressors::compressor_type = Compressors::IDOCOMPMAX;
        else {
            std::cerr << "The compressor is not supported. Available options: rlz, gdc2, hirgc, sccg" << std::endl;
            exit(1);
        }
    }

    if (!std::filesystem::is_regular_file(met_path / "metadata")) {
        std::cerr << "first argument is not a valid metadata directory" << std::endl;
        std::exit(1);
    }
    compress_hierarchy(met_path, out_folder);

    return 0;
}
void compress_hierarchy(const std::filesystem::path& met_path, const std::filesystem::path& out_folder, const std::string& metadata_name) {
    //Two tasks: Read external metadata to get the files and the arboricity, and compress
    std::unique_ptr<ExternalMetadata> met = read_external_metadata(met_path, metadata_name);
    int n = met->n_sequences;
    std::filesystem::path origin = met->original_directory;
    std::string *filenames = met->filenames.get();
    DynArray<std::filesystem::path> files(n);
    for (int i = 0; i < n; i++) {
        files[i] = origin / filenames[i];
    }
    std::size_t root = met->root_id;

    //Compress
    ProfilerSteady::profile("compress_to_file", [&]() {
        std::unique_ptr<Compressors::Compressor> comp = Compressors::getCompressor();
        total_size = comp->compress_dataset(files, met->arbor, root, out_folder);
    });
    //Finish
    Compressors::cleanup();

    /* We end up by reporting the results. */
    root_size = filesystem::file_size(files[root]);
    /* Finish timing */
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    //Pretty print basic data
    std::vector<alignment> al {LEFT, LEFT, RIGHT, RIGHT};
    std::vector<std::string> header {"Dataset", "Root ID", "Compressed size", "Compression time"};
    size_t size = root_size + total_size;
    std::vector<std::string> data {origin.filename(), std::to_string(root), format_size(size), format_time(elapsed_seconds.count() * 1000000000)};
    std::cout << "Table of data:\n" << std::endl; //Two newlines
    print_header(std::cout, header, al);
    print_row(std::cout, data);
    //Pretty print timers
    ProfilerSteady::report(std::cout);
}
