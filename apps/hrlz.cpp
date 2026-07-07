#include<algorithm>
#include<utility>
#include<vector>
#include<cmath>
#include<iostream>
#include<cstring>
#include<fstream>
#include<cassert>
#include <ctime>
#include <chrono>
#include <random>
#include <mutex>
#include <thread>
#include <omp.h>

#include "hrlz/file.hpp"
#include "hrlz/ArrayHash.hpp"
#include "hrlz/Fingerprint.hpp"
#include "hrlz/Cluster.hpp"
#include "hrlz/BFS.hpp"
#include "rlz/rlz.hpp"
#include "common/SteadyTimer.hpp"
#include "hrlz/Compressors.hpp"
#include "utils/PrettyPrint.hpp"
#include "utils_tests.hpp"
#include "hrlz/decompress_seq.hpp"
#include "HuffmanFile.hpp"

bool complete = false;
bool normal_rlz = false;
bool test_rels = false;
float percent_rels = 0.0;
std::size_t rlz_root = 0;
bool decompress_sequence = false;
std::size_t decompress_sequence_index = 0;
bool compress = false;
bool decompress = false;
char* input_file = nullptr;
char* decompress_source = nullptr;
char *chosen_output_folder = nullptr;
std::vector<WeightedEdge> arbor;
//To save and use an existing clustering
bool print_graph = false;
bool load_graph = false;
char *graph_file = nullptr;
std::ifstream graph_file_stream;
//Max: last parameters
size_t total_size = 0;
size_t root_size = 0;
std::vector<size_t> root_ids {};
std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> start;
//To divide the compression into chromosomes
bool chromosomes = false;
char *chr_name_file = nullptr;
std::vector<std::string> chr_names = {"chr1", "chr2", "chr3", "chr4", "chr5", "chr6", "chr7", "chr8", "chr9",
                                          "chr10", "chr11", "chr12", "chr13", "chr14", "chr15", "chr16", "chr17",
                                          "chr18", "chr19", "chr20", "chr21", "chr22", "chrX", "chrY"};
const std::size_t SHINGLE_SIZE = 256;
std::size_t FINGERPRINT_SIZE = 4;
const std::size_t ROUNDS_UNTIL_SHRINK = 10;
int max_threads = 0;
bool rels_parallel = false;
bool test_seqs = false;
float percent_seqs = 0;
bool approx_hierarchy = false;
bool bottom_k = false;
const size_t approx_fingerprint_size = 1024;
int dataset;

enum thresholds {
    TWO_SQRT, TWO_LOG
};
enum thresholds threshold = TWO_LOG;

std::size_t cluster_threshold(std::size_t n) {
  if (threshold == TWO_LOG) {
      return 2 * std::floor(std::log2(n));
  } else { //TWO_SQRT
      return 2 * std::sqrt(n);
  }
}

void report_edges(std::ostream& out, const std::vector<WeightedEdge>& edges, std::size_t root, std::size_t root_file_size) {
  total_size = 0;
  for(auto edge: edges) {
    total_size += edge.weight;
  }
  out << "Number of edges: " << edges.size() << ", root is index " << root << std::endl;
  out << "The sum of weights is " << total_size << ", the root file size is " << root_file_size << std::endl;
  out << "The edges of the arborescence are: " << std::endl;
  for(auto edge: edges) {
    out << edge.from << " " << edge.to << " " << edge.weight << std::endl;
  }
}

void report_metadata(std::ostream& out, const DynArray<SequenceMetadata>& metadatas, std::size_t root, std::size_t root_file_size) {
  std::size_t sum = 0;
  for(auto metadata: metadatas) {
    sum += metadata.factors;
  }

  out << metadatas.size() - 1 << " " << root << " " << sum << " " << root_file_size << std::endl;
  for(std::size_t i = 0; i < metadatas.size(); ++i) {
    if(i == root)
      continue;
    auto& metadata = metadatas[i];
    out << metadata.parent << " " << i << " " << metadata.factors << std::endl;
  }
}

size_t normal_rlz_star_external(const DynArray<std::filesystem::path>& files, const DynArray<std::filesystem::path>& sa_files,
                                size_t root, const std::filesystem::path& output_dir) {
    std::size_t n = files.size();
    size_t size = 0;
    //Make arbor
    std::unique_ptr<std::vector<std::size_t>[]> arbor_ptr =
            std::make_unique<std::vector<std::size_t>[]>(files.size());
    std::vector<std::size_t> *adj_list = arbor_ptr.get();
    //Create the vectors
    for (std::size_t i = 0; i < n; i++) {
        adj_list[i] = std::vector<std::size_t>();
    }
    //Only the root has adjacents
    std::vector<std::size_t>& root_adj = adj_list[root];
    for (std::size_t i = 0; i < root; i++) {
        root_adj.push_back(i);
    }
    for (std::size_t i = root + 1; i < n; i++) {
        root_adj.push_back(i);
    }
    std::unique_ptr<Compressors::Compressor> comp = Compressors::getCompressor();
#ifndef NDEBUG
    comp->print_stats = true;
#else
    comp->print_stats = false;
#endif
    size = comp->compress_dataset(files, arbor_ptr, root, output_dir);
    return size;
}

std::vector<WeightedEdge> best_star(const std::set<WeightedEdge>& edges, std::size_t n, std::size_t& root) {
  DynArray<std::size_t> sums(n);
  for(std::size_t i = 0; i < n; ++i) {
    sums[i] = 0;
  }
  for(auto edge: edges) {
    sums[edge.from] += edge.weight;
  }
  std::size_t best = 0;
  std::size_t best_sum = sums[0];
  std::cout << "Normal rlz root and factor count" << std::endl;
  for(std::size_t i = 0; i < n; ++i) {
    std::cout << i << " " << sums[i] << std::endl;
    if(sums[i] < best_sum) {
      best_sum = sums[i];
      best = i;
    }
  }
  arbor.reserve(n-1);
  root = best;
  for(auto edge: edges) {
    if(edge.from == best)
      arbor.push_back(edge);
  }
  return arbor;
}

std::unique_ptr<AdjacencyList> complete_graph(const DynArray<std::filesystem::path>& files) {
  std::unique_ptr<AdjacencyList> complete = std::make_unique<AdjacencyList>(files.size());
  std::cout << "Reserving memory..." << std::endl;
  //Reserve memory
  for (std::size_t i = 0; i < files.size(); i++) {
    complete->reserve_for_i(i, files.size() - 1);
  }
  std::cout << "Adding edges..." << std::endl;
  for(std::size_t i = 0; i < files.size(); ++i ){
    for(std::size_t j = i+1; j < files.size(); ++j ){
      complete->add_edge(i,j);
      complete->add_edge(j,i);
    }
  }
  return complete;
}

std::unique_ptr<AdjacencyList> cluster_graph(const DynArray<std::filesystem::path>& files) {
  ClusterSettings cs;
  DynArray<std::filesystem::path> tmp(files.size());
  for(std::size_t i = 0; i < files.size(); ++i) {
    tmp[i] = files[i];
  }
  cs.files = std::move(tmp);
  cs.shingle_size = SHINGLE_SIZE;
  cs.fingerprint_size = FINGERPRINT_SIZE;
  cs.rounds_until_shrink = ROUNDS_UNTIL_SHRINK;
  cs.cutoff = cluster_threshold(cs.files.size());
  cs.bottom_k = bottom_k;
  ClusterGraph *g = compute(cs);
  std::unique_ptr<AdjacencyList> adj = g->take_adjacencyList();
  delete g;
  return adj;
}

int test_rels_loop(const DynArray<std::filesystem::path>& files) {
    DynArray<std::filesystem::path> sa_files(files.size());
    for(std::size_t i = 0; i < files.size(); ++i ){
        sa_files[i] = std::string(files[i]) + ".sa";
    }
    //We're gonna call normal_rlz_external for a random subset of chosen roots, to a temporary directory.
    //Time it and get the size. Then, we are going to decompress it, timed, to a temporary directory,
    //and pass the compare_fasta to see if it is correct. Then, we are going to output a table with
    //the results. ProfilerSteady::Profile resets the counter each time.
    //Temp output dir
    //Get random sequence
    size_t n = files.size();
    size_t amount = std::ceil((percent_rels * n) / 100.0);
    std::cout << "Testing " << amount << " random roots..." << std::endl;
    std::vector<size_t> ns, sampled;
    ns.reserve(n);
    sampled.reserve(amount);
    for (size_t i = 0; i < n; i++)
        ns.push_back(i);
    //Sample without replacement (see https://en.cppreference.com/w/cpp/algorithm/sample)
    //We use minstd_rand cuz it's faster and tbh we don't mind that much about stat properties here
    std::sample(ns.begin(), ns.end(), std::back_inserter(sampled), amount,
                std::minstd_rand { GlobalRandom::get() });
    size_t best_root = -1;
    size_t best_size = ULLONG_MAX; //max unsigned long
    size_t best_i = ULLONG_MAX;
    std::stringstream output;
    //output << "Root;Size;Compression time;Decompression time;Size (B);Compression time (ns);Decompression time(ns)\n";
    output << "Root;Size;Compression time;Size (B);Compression time (ns)\n";
    size_t total_time_compression = 0;
//    size_t total_time_decompression = 0;
    //And compute
    std::filesystem::path temp_dir;
    switch (Compressors::compressor_type) {
        case Compressors::RLZ:
            temp_dir = std::filesystem::current_path() / ("temp_rels_RLZ");
            break;
        case Compressors::IDOCOMPMAX: {
            temp_dir = std::filesystem::current_path() / ("temp_rels_idocompmax");
            break;
        }
        case Compressors::HIRGCMAX: {
            temp_dir = std::filesystem::current_path() / ("temp_rels_hirgcmax");
            break;
        }
    }
    if (!std::filesystem::exists(temp_dir)) {
        std::filesystem::create_directories(temp_dir);
    }
    std::string prefix;
    switch (Compressors::compressor_type) {
        case Compressors::HIRGCMAX: {
            prefix = "comp_rel_hrlz_hirgcmax_";
            break;
        }
        case Compressors::IDOCOMPMAX: {
            prefix = "comp_rel_hrlz_idocompmax_";
            break;
        }
        case Compressors::RLZ : {
            prefix = "comp_rel_hrlz_rlz_";
            break;
        }
    }
//#pragma omp parallel for if(rels_parallel)
    for (size_t i = 0; i < amount; i++) {
        //Directories
        std::filesystem::path output_compression = temp_dir / (prefix + std::to_string(i));
        if (!std::filesystem::exists(output_compression))
            std::filesystem::create_directory(output_compression);
        //Compression
        size_t size = 0;
        size_t root_id = sampled[i];
        SteadyTimer timer_i;
        timer_i.restart();
        try {
            size = normal_rlz_star_external(files, sa_files, root_id, output_compression);
            size += filesystem::file_size(files[root_id]);
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            std::exit(1);
        }
        timer_i.stop();
        size_t c_time = timer_i.duration().count();
        std::string data = std::to_string(root_id) + ";" + format_size(size) + ";" + format_time(c_time)
                + ";" + std::to_string(size) + ";" + std::to_string(c_time) + "\n";
//#pragma omp critical
//        {
        std::cout << data;
        output << data;
        total_time_compression += c_time;
//        total_time_decompression += d_time;
        if (size < best_size) {
            best_size = size;
            best_root = root_id;
            if (best_i != ULLONG_MAX) {
                std::filesystem::remove_all(temp_dir / (prefix + std::to_string(best_i)));
            }
            best_i = i;
        } else {
            //Delete files
            std::filesystem::remove_all(output_compression);
        }
//        }
//        std::filesystem::remove_all(output_decompression); //nice
    }
    std::cout << "Finished testing roots" << std::endl;
    std::filesystem::path output_compression = temp_dir / (prefix + std::to_string(best_i));
    if (compress) {
        for (const auto& entry : std::filesystem::directory_iterator(output_compression)) {
            if (entry.is_regular_file()) {
                const std::filesystem::path& p = entry.path();
                std::filesystem::rename(p, std::filesystem::path(chosen_output_folder) / p.filename());
            }
        }
    }
    std::filesystem::remove_all(output_compression);
    /* Finish timing */
    auto end = std::chrono::system_clock::now();
    std::cout << "Full output table: " << std::endl;
    std::cout << output.rdbuf();
    std::cout << "Best root: " << best_root << ", computed size: " << best_size << ", pretty printed: " << format_size(best_size) << std::endl;
    std::cout << "Total time compression: " << total_time_compression << " ns, in time: " << format_time(total_time_compression) << std::endl;
//    std::cout << "Total time decompression: " << total_time_decompression << " ns, in time: " << format_time(total_time_decompression) << std::endl;
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    return 0;
}

int test_decompress_sequence(const DynArray<std::filesystem::path>& files, float percent_seqs) {
    size_t n = files.size();
    size_t amount = std::max(5UL, static_cast<size_t>(std::ceil((percent_seqs * n) / 100.0)));
    std::vector<size_t> ns, sampled;
    ns.reserve(n);
    sampled.reserve(amount);
    for (size_t i = 0; i < n; i++)
        ns.push_back(i);
    //Sample without replacement (see https://en.cppreference.com/w/cpp/algorithm/sample)
    //We use minstd_rand cuz it's faster and tbh we don't mind that much about stat properties here
    std::sample(ns.begin(), ns.end(), std::back_inserter(sampled), amount,
                std::minstd_rand { GlobalRandom::get() });
    std::cout << "Testing " << amount << " random sequences..." << std::endl;
    size_t time_all = 0;
    const std::filesystem::path& dec_source = std::filesystem::canonical(decompress_source);
    std::mutex lock;
#pragma omp parallel for
    for (size_t i = 0; i < amount; i++) {
        SteadyTimer t;
        t.start();
        //Decompress
#ifndef NDEBUG
        std::cout << "Decompressing sequence no. " << sampled[i] << std::endl;
#endif
        std::unique_ptr<Compressors::Compressor> comp = Compressors::getCompressor();
        std::filesystem::path temp_dir = filesystem::output_dir / (".hrlz_dec_sequences_" + std::to_string(sampled[i]));
        if (!std::filesystem::exists(temp_dir)) {
            std::filesystem::create_directories(temp_dir);
        }
        comp->decompress_sequence(dec_source, sampled[i], temp_dir);
        t.stop();
        size_t time = t.duration().count();
        //Delete
        std::filesystem::remove(temp_dir / (files[sampled[i]]).filename());
        std::filesystem::remove(temp_dir);
        lock.lock();
#ifndef NDEBUG
            std::cout << "Adding time" << std::endl;
#endif
            time_all += time;
#ifndef NDEBUG
            std::cout << "Finished adding time" << std::endl;
#endif
        lock.unlock();
    }

    //Finish
    std::cout << "Total time to decompress the " << amount << " sequences: " << time_all << ", formatted: " << format_time(time_all) << std::endl;
    size_t avg = time_all / amount;
    std::cout << "Average time: " << avg << ", formatted: " << format_time(avg) << std::endl;
    //Fine-grained timings
    std::cout << "Total time to load sequences: ";
    size_t load_time = 0;
    for (int i = 0; i < max_threads; i++) {
        load_time += ProfilerSteady::get_time("load_ref_" + std::to_string(i));
    }
    std::cout << load_time << ", human-readable: " << format_time(load_time) << std::endl;
    std::cout << "Total time to unzip files: ";
    size_t unzip_time = 0;
    for (int i = 0; i < max_threads; i++) {
        unzip_time += ProfilerSteady::get_time("decode_target_" + std::to_string(i));
    }
    std::cout << unzip_time << ", human-readable: " << format_time(unzip_time) << std::endl;
    std::cout << "Decompress time: ";
    size_t decomp_time = 0;
    for (int i = 0; i < max_threads; i++) {
        decomp_time += ProfilerSteady::get_time("decompress_target_" + std::to_string(i));
    }
    std::cout << decomp_time << ", human-readable: " << format_time(decomp_time) << std::endl;
    return 0;
}

int finish_compress(const DynArray<std::filesystem::path>& files) {
    /*Compressions are done, cleanup */
    std::cout << "Cleaning up" << std::endl;
    Compressors::cleanup();
    /* We end up by reporting the results. */
    int n_roots = root_ids.size();
    std::cout << "N roots: " << n_roots << std::endl;
    root_size = 0;
    if (chromosomes) {
        if (normal_rlz) {
            for (int i = 0; i < n_roots; i++)
                root_size += filesystem::file_size(files[root_ids[0]] / chr_names[0]);
        } else {
            std::cout << "Calculating root sizes (hrlz):" << std::endl;
            for (int i = 0; i < n_roots; i++) {
		        std::cout << "Root: " << root_ids[i] << std::endl;
                std::filesystem::path r = files[root_ids[i]] / chr_names[i];
		        std::cout << "Exploring root " << r.string() << std::endl;
                root_size += filesystem::file_size(r);
            }
        }
    } else {
        root_size = filesystem::file_size(files[root_ids[0]]);
    }
    /* Finish timing */
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << std::endl;
    //Pretty print basic data
    std::vector<alignment> al {LEFT, LEFT, RIGHT, RIGHT, RIGHT, RIGHT};
    std::vector<std::string> header {"Dataset", "Method", n_roots == 1 ? "Root ID" : "Root IDs", "Compressed size", "Compression time", "Number of cores"};
    std::string method;
    if (complete)
        method = "Complete";
    else if (normal_rlz)
        method = "Star graph";
    else
        method = "Hierarchical";
    size_t size = root_size + total_size;
    std::string root_names = std::to_string(root_ids[0]);
    std::cout << "Creating string for roots" << std::endl;
    for (int i = 1; i < n_roots; i++)
        root_names += ", " + std::to_string(root_ids[i]);
    std::cout << "Creating data... " << std::endl;
    std::vector<std::string> data {input_file, method, root_names, format_size(size), format_time(elapsed_seconds.count() * 1000000000), std::to_string(max_threads)};
    std::cout << "Table of data:\n" << std::endl; //Two newlines
    print_header(std::cout, header, al);
    print_row(std::cout, data);
    //Print sequential times
    size_t seq_time_cluster;
    if (compress && (!complete) && (!load_graph) && (max_threads > 1)) {
        //We computed the clustering
        size_t time_threads = 0;
        for (int i = 0; i < max_threads; i++) {
            time_threads += ProfilerSteady::get_time("runround_" + std::to_string(i));
        }
        size_t parallel_runs = ProfilerSteady::get_time("runround_sequential");
        size_t total_time = ProfilerSteady::get_time("construct_graph");
        seq_time_cluster = total_time - parallel_runs + time_threads;
        std::cout << "Sequential time for constructing graph: " << seq_time_cluster << ", pretty printed: " << format_time(seq_time_cluster) << std::endl;
    }
    size_t time_weights = 0;
    if (compress && (!normal_rlz) && (max_threads > 1)) {
        //get the thread times
        size_t thread_time = 0;
        for (int i = 0; i < max_threads; i++) {
            thread_time += ProfilerSteady::get_time("compute_edges_" + std::to_string(i));
        }
        std::cout << "Sequential time for computing real distances: " << thread_time << ", pretty printed: " << format_time(thread_time) << std::endl;
        time_weights = thread_time;
    }
    //Pretty print timers
    ProfilerSteady::report(std::cout);
    return 0;
}

int finish_decompress() {
    ProfilerSteady::report(std::cout);
    return 0;
}

std::unique_ptr<AdjacencyList> load_adj(size_t n) {
    std::unique_ptr<AdjacencyList> adj = std::make_unique<AdjacencyList>(n);
    std::string line;
    for (size_t i = 0; i < n; i++) {
        std::getline(graph_file_stream, line);
        std::istringstream iss(line);
        size_t id;
        iss >> id;
        iss.get(); iss.get(); //Remove the ->
        size_t j;
        while (iss >> j) {
            adj->add_edge(id, j);
        }
    }
    return adj;
}

void hrlz_compress(const DynArray<std::filesystem::path>& files, int chr=0);
int main(int argc, char** argv) {
    start = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t(start);
    std::cout << "Started the computation at " << std::ctime(&start_time) << std::endl;
    max_threads = omp_get_max_threads();
    std::cout << "Maximum number of threads: " << max_threads << std::endl;
    for (int i = 0; i < argc; i++) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
        std::cout << "Argument " << i << " has type " << typeid(argv[i]).name() << std::endl;
    }
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--opt") == 0) {
            complete = true;
        } else if (strcmp(argv[i], "--rlz") == 0) {
            normal_rlz = true;
            ++i;
            if (argc < i) {
                std::cerr << "Missing root for normal rlz after --rlz" << std::endl;
            }
            rlz_root = std::stoi(argv[i]);
        }
            //This option is to specify a percentage of random stars (among all possible stars) to test
        else if (strcmp(argv[i], "--test_rels") == 0) {
            test_rels = true;
            i++;
            if (argc < i) {
                std::cerr << "Missing percentage of stars to test" << std::endl;
                std::exit(1);
            }
            try {
                percent_rels = std::stof(argv[i]);
                if (percent_rels < 0 || percent_rels > 100) {
                    std::cerr << "Please enter a valid percentage (from 0 to 100) of stars to test" << std::endl;
                    std::exit(1);
                }
            } catch (std::invalid_argument &e) {
                std::cerr << "Invalid percentage of the stars to test" << std::endl;
                std::exit(1);
            }
        } else if (strcmp(argv[i], "--test_sequences") == 0) {
            test_seqs = true;
            i++;
            if (argc < i) {
                std::cerr << "Missing percentage of sequences to test" << std::endl;
                std::exit(1);
            }
            try {
                percent_seqs = std::stof(argv[i]);
                if (percent_seqs < 0 || percent_seqs > 100) {
                    std::cerr << "Please enter a valid percentage (from 0 to 100) of stars to test" << std::endl;
                    std::exit(1);
                }
            } catch (std::invalid_argument &e) {
                std::cerr << "Invalid percentage of the stars to test" << std::endl;
                std::exit(1);
            }
        }
            //This option is to specify an output file for compression
        else if (strcmp(argv[i], "-c") == 0) {
            ++i;
            compress = true;
            if (argc < i) {
                std::cerr << "Missing argument after -c" << std::endl;
                exit(1);
            }
            chosen_output_folder = argv[i];
        }
        //This option is to specify the clustering threshold function (default is 2log n)
        else if (strcmp(argv[i], "--function") == 0) {
            ++i;
            if (argc < i) {
                std::cerr << "Missing argument after --function" << std::endl;
                exit(1);
            }
            if (strcmp(argv[i], "2sqrt") == 0) {
                threshold = TWO_SQRT;
            } else if (strcmp(argv[i], "2log") == 0) {
                threshold = TWO_LOG;
            } else {
                std::cerr << "Function not recognized or implemented: " << argv[i] << std::endl;
                exit(1);
            }
        }
            //Specifies that we are decompressing, otherwise we are compressing
        else if (strcmp(argv[i], "-d") == 0) {
            ++i;
            decompress = true;
            if (argc < i) {
                std::cerr << "Missing argument after -d" << std::endl;
                exit(1);
            }
            decompress_source = argv[i];
        }
            //To decompress a single index
        else if (strcmp(argv[i], "-s") == 0) {
            ++i;
            decompress_sequence = true;
            if (argc < i) {
                std::cerr << "Missing argument after -s" << std::endl;
                exit(1);
            }
            decompress_sequence_index = std::stoi(argv[i]);
        }
            //To specify a different algorithm for compression
        else if (strcmp(argv[i], "--compressor") == 0) {
            if (argc <= i + 1) {
                std::cout << "Missing argument for the compressor (default is RLZ)" << std::endl;
                exit(1);
            }
            i++;
            std::string selected_compressor = argv[i];
            //to lower case (from stack overflow)
            std::transform(selected_compressor.begin(), selected_compressor.end(), selected_compressor.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            //Seems that C++ doesn't support switch statements on strings.
            if (selected_compressor == "rlz") Compressors::compressor_type = Compressors::RLZ;
            else if (selected_compressor == "hirgc") Compressors::compressor_type = Compressors::HIRGCMAX;
            else if (selected_compressor == "idocomp") Compressors::compressor_type = Compressors::IDOCOMPMAX;
            else {
                std::cerr
                        << "The compressor is not supported. Available options: rlz, hirgc, idocomp"
                        << std::endl;
                exit(1);
            }
        }
            //To specify an output folder
        else if (strcmp(argv[i], "-o") == 0) {
            //We are specifying an output folder
            if (argc <= i + 1) {
                std::cerr << "Missing output folder (default is ./output_dir)" << std::endl;
                exit(1);
            }
            i++;
            chosen_output_folder = argv[i];
        } else if (strcmp(argv[i], "--print_graph") == 0) {
            print_graph = true;
        } else if (strcmp(argv[i], "--load_graph") == 0) {
            load_graph = true;
            if (argc <= i + 1) {
                std::cerr << "Missing graph file" << std::endl;
                exit(1);
            }
            i++;
            graph_file = argv[i];
            graph_file_stream.open(graph_file);
            if (!graph_file_stream) {std::cerr << "Couldn't open graph file" << std::endl; std::exit(1);}
        } else if (strcmp(argv[i], "--random") == 0) {
            if (argc <= i + 1) {
                std::cerr << "Missing random seed!!!" << std::endl;
                exit(1);
            }
            uint64_t r = std::stoull(argv[++i]);
            GlobalRandom::set_global_seed(r);
        } else if (strcmp(argv[i], "--sketch_size") == 0) {
            if (argc <= i + 1) {
                std::cerr << "Missing sketch size!!!" << std::endl;
                exit(1);
            }
            FINGERPRINT_SIZE = std::stoull(argv[++i]);
        } else if (strcmp(argv[i], "--rels_parallel") == 0) {
            rels_parallel = true;
        } else if (strcmp(argv[i], "--bottom_k") == 0) {
            bottom_k = true;
        } else if (strcmp(argv[i], "--jaccard") == 0) {
            approx_hierarchy = true;
        } else {
            input_file = argv[i];
        }
    }
    std::cout << "Threshold function: " << (threshold == TWO_LOG ? "2log n" : "2sqrt(n)") << std::endl;
    if (decompress && compress) {
        std::cerr << "Cannot compress and decompress at the same time" << std::endl;
        exit(1);
    }

    //Now that we have the input file, we can init the output folder in the same location
    if (chosen_output_folder == nullptr) {
        std::filesystem::path file_path = input_file;
        filesystem::init_output_folder(file_path.parent_path() / "output_dir");
    } else {
        filesystem::init_output_folder(chosen_output_folder);
    }

    if (decompress && !decompress_sequence && !test_seqs) {
        ProfilerSteady::profile("decompress", [&]() {
            std::ifstream input_file;
            std::filesystem::path decomp_path = std::filesystem::canonical(decompress_source);
            std::unique_ptr<Compressors::Compressor> comp = Compressors::getCompressor();
            comp->decompress_dataset(decomp_path, filesystem::output_dir);
        });
        return finish_decompress();
    }
    if (decompress && decompress_sequence && !test_seqs) {
        ProfilerSteady::profile("decompress_sequence", [&]() {
            std::unique_ptr<Compressors::Compressor> comp = Compressors::getCompressor();
            comp->decompress_sequence(std::filesystem::canonical(decompress_source), decompress_sequence_index,
                                      filesystem::output_dir);
        });
        return finish_decompress();
    }

    if (input_file == nullptr) {
        std::cerr << "No input file specified" << std::endl;
        exit(1);
    }

    auto files = read_input_file(input_file);
    //Check if we need to run test stars or hierarchy
    if (test_rels) {
        return test_rels_loop(files);
    }
    if (test_seqs) {
        return test_decompress_sequence(files, percent_seqs);
    }
    if (!chromosomes && !normal_rlz) {
        hrlz_compress(files);
        return finish_compress(files);
    } else {
        //Check if we need to run RLZ instead of hierarchy
        if(normal_rlz && !complete) {
            DynArray<std::filesystem::path> sa_files(files.size());
            for(std::size_t i = 0; i < files.size(); ++i ){
                sa_files[i] = std::string(files[i]) + ".sa";
            }
            ProfilerSteady::profile("normal_rlz", [&](){
                total_size = normal_rlz_star_external(files, sa_files, rlz_root, filesystem::output_dir);
            });
            root_ids.push_back(rlz_root);
            return finish_compress(files);
        }
        //For each file, compress
        size_t n_chromosomes = chr_names.size();
        for (size_t i = 0; i < n_chromosomes; i++) {
            size_t n = files.size();
            DynArray<std::filesystem::path> chr(n);
            for (size_t j = 0; j < n; j++) {
                chr[j] = files[j] / chr_names[i];
            }
            hrlz_compress(chr, i);
        }
	std::cout << "Finishing compression" << std::endl;
        return finish_compress(files);
    }
}

void hrlz_compress(const DynArray<std::filesystem::path>& files, int chr) {
  DynArray<std::filesystem::path> sa_files(files.size());
  for(std::size_t i = 0; i < files.size(); ++i ){
    sa_files[i] = std::string(files[i]) + ".sa";
  }

  std::unique_ptr<std::set<WeightedEdge>> edges;

  std::unique_ptr<AdjacencyList> adj;
  ProfilerSteady::profile("construct_graph", [&]() {
    if (load_graph) {
        //We're gonna read the graph from the file
        std::cout << "Loading graph..." << std::endl;
        adj = load_adj(files.size());
        std::cout << "Graph loaded" << std::endl;
    }
    else if(complete) {
        std::cout << "Building complete graph..." << std::endl;
        adj = complete_graph(files);
        std::cout << "No. of vertices: " << adj->node_count() << std::endl;
        std::cout << "No. of edges: " << adj->edge_count() << std::endl;
    }
    else {
        /**
         * Construct the graph data structure. Either complete or clustered.
         * If it is meant to be sparsified, we call the clustering procedure (cluster_graph)
         * to sparsify the graph.
        */
        std::cout << "Size of k-mer: " << SHINGLE_SIZE << std::endl;
        std::cout << "Size of fingerprint: " << FINGERPRINT_SIZE << std::endl;
        std::cout << "No. of rounds till shrink: " << ROUNDS_UNTIL_SHRINK << std::endl;
        adj = cluster_graph(files);
        /**
         * Print graph
         */
        std::cout << "Sparsified graph:" << std::endl;
        std::cout << "No. of vertices: " << adj->node_count() << std::endl;
        std::cout << "No. of edges: " << adj->edge_count() << std::endl;
    }
  }, chr==0); //restart timer only if it's the first chromosome, otherwise append time
  //Report intermediate results
  ProfilerSteady::report(std::cout);
  /**
   * Print the graph if requested
   * */
   if (print_graph) {
       ProfilerSteady::profile("Printing graph", [&]() {
           std::cout << "Printing graph..." << std::endl;
           std::ofstream g(filesystem::output_dir / "graph.txt", std::ios_base::app);
           for (size_t i = 0; i < files.size(); i++) {
               auto set = adj->adjacent(i);
               g << i << "->";
               if (!set.empty()) {
                   auto it = set.begin();
                   g << *it++;
                   while (it != set.end()) {
                       g << " " << *it++;
                   }
               }
               g << std::endl;
           }
           g.close();
       }, chr==0); //append to timer
   }

  /**
   * We have the graph sparsified (if needed). Then, on every edge of the graph, we compute
   * the relative compression weight using the compressors
  */

  //edges will be the set of edges of the sparse graph
  ProfilerSteady::profile("compute_edge_weights", [&]() {
      std::cout << "Computing edge weights..." << std::endl;
      if (approx_hierarchy) {
          edges = compute_approx_weights(*adj, files, SHINGLE_SIZE, approx_fingerprint_size);
      } else {
          edges = compute_rlz_for_edges(*adj, files, sa_files);
      }
  }, chr==0); //append to timer

  /**
   * Now, we have the weight of the edges computed. We don't need the adjacency list anymore,
   * since we have a set of weighted edges that already encodes the graph in the variable "edges"
  */
  adj.reset();

  /**
   * The next step is to compute the best arborescence on this set of edges.
   * If we are using normal RLZ without hierarchy, then we compute the best star.
  */
  //Report intermediate timings
  ProfilerSteady::report(std::cout);
  if(complete) { // Just compute the star as well and output result to stdout
    std::size_t star_root;
    std::vector<WeightedEdge> star = best_star(*edges, files.size(), star_root);
    std::cout << "About to report star result" << std::endl;
    report_edges(std::cout, star, star_root, filesystem::file_size(files[star_root]));
  }
  size_t root_id;
  ProfilerSteady::profile("arborescence", [&]() {
    if(normal_rlz) {
      arbor = best_star(*edges, files.size(), root_id);
    }
    else {
      std::cout << "Finding minimum spanning arborescence on graph with " << files.size() << " nodes and " << edges->size() << " edges" << std::endl;
      arbor = arborescence(*edges, root_id, files.size());
    }
  }, chr==0); //append to timer

  /**
   * Now we have the set of edges that forms the hierarchy in "arbor". We don't need the previous
   * set of edges anymore ("edges"), so we delete it.
  */

  edges.reset();

  //We make the adjacency list of arbor
  std::cout << "Making arboricity for hrlz " << chr << std::endl;
  std::unique_ptr<std::vector<std::size_t>[]> adj_list_ptr =
          std::make_unique<std::vector<std::size_t>[]>(files.size());
  std::vector<std::size_t> *adj_list = adj_list_ptr.get();
  for (std::size_t i = 0; i < files.size(); i++) {
      adj_list[i] = std::vector<std::size_t>();
  }
  for (WeightedEdge edge: arbor) {
      adj_list[edge.from].push_back(edge.to);
  }
  //We pass it
  std::cout << "Compressing to file, chromosome " << chr << std::endl;
  if (compress) {
      ProfilerSteady::profile("compress_to_file", [&]() {
          std::unique_ptr<Compressors::Compressor> comp = Compressors::getCompressor();
          comp->print_stats = true;
          total_size += comp->compress_dataset(files, adj_list_ptr, root_id, filesystem::output_dir);
      }, chr==0);
  }
  std::cout << "Finished compressing to file chromosome " << chr << std::endl;

  root_ids.push_back(root_id);
  std::cout << "Pushed back root id " << root_id << std::endl;
}

/**
 * LEGACY CODE THAT MIGHT BE USEFUL LATER
 *


//Print individual edge weights

ProfilerSteady::profile("report_result", [&]() {
report_edges(std::cout, arbor, root_id, root_size);
});



   //RLZ uses BFS order pre-computed. The other compression schemes do it themselves

  if (Compressors::compressor_type == Compressors::RLZ) {

      // The next step is to just order the edges of the arboricity in a BFS order.

std::vector<Edge> bfs_ordered_edges;

ProfilerSteady::profile("bfs_order", [&]() {
bfs_ordered_edges = bfs_order_edges(arbor, files.size(), root_id);
}, chr==0);


 //Once we have this, we can write to a file the compressed data if specified.

if(compress) {
ProfilerSteady::profile("compress_to_file", [&]() {
std::ofstream output_file_stream;
std::string output_filename = filesystem::output_dir / "output";
output_file_stream.open(output_filename);
compress_to_stream(output_file_stream, files, bfs_ordered_edges, root_id, sa_files);
output_file_stream.close();
}, chr==0);
}
} else {


 ---
             if (Compressors::compressor_type == Compressors::RLZ) {
                //We assume that if we are using RLZ, the output path is a file. If not,
                //we append the file.
                if (std::filesystem::is_directory(decomp_path)) {
                    decomp_path /= "output";
                }
                //RLZ code to decompress
                input_file.open(decomp_path);
                if (decompress_sequence) {
                    decompress_sequence_from_stream(input_file, decompress_sequence_index);
                } else {
                    decompress_from_stream(input_file);
                }
                input_file.close();
            } else {

 -------

                if (Compressors::compressor_type == Compressors::RLZ){
                    normal_rlz_star(files, sa_files);
                    //I want to change this return
                    ProfilerSteady::report(std::cout);
                    std::exit(0);
                }
                else {



void normal_rlz_star(const DynArray<std::filesystem::path>& files, const DynArray<std::filesystem::path>& sa_files) {
  auto root = rlz_root;
  arbor.reserve(files.size() - 1);
  if(compress) {
      //I want to change this reporting
    std::vector<Edge> edges;
    edges.reserve(files.size() - 1);
    for(std::size_t i = 0; i < files.size(); ++i) {
      if(i==root)
        continue;
      edges.emplace_back(root, i);
    }
    std::ofstream output_file;
    std::filesystem::path compress_output = filesystem::output_dir / "output";
    output_file.open(compress_output);
    auto metadatas = compress_to_stream(output_file, files, edges, root, sa_files);
    output_file.close();
    report_metadata(std::cout, metadatas, root, filesystem::file_size(files[root]));
  }
  else {
    RLZCompress compressor = RLZCompress::from(files[0], sa_files[0]);
    for(std::size_t i = 1; i < files.size(); ++i) {
      FactorSink writer;
      compressor.compress(files[i].c_str(), writer);
      arbor.emplace_back(0,i, writer.factor_count());
    }
    //Reporting the edges will be done at finishing time
  }
}

*/