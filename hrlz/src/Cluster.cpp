#include <hrlz/Cluster.hpp>

#include "hrlz/file.hpp"
#include "hrlz/ArrayHash.hpp"
#include "hrlz/Fingerprint.hpp"
#include "hrlz/Cluster.hpp"
#include "rlz/rlz.hpp"
#include "hrlz/Compressors.hpp"
#include "common/SteadyTimer.hpp"
#include <algorithm>

#include <utility>
#include <mutex>
#include <omp.h>


void handle_cluster(const ClusterSettings &cs, const std::vector<std::size_t> &cluster, ClusterGraph &g, DynArray<std::size_t> &collisions)
{
  for (const auto &v : cluster)
  {
    collisions[v] += cluster.size();
  }
  //If the cluster is smaller than the cutoff, then we add the edges.
  //Otherwise, the cluster is too big and we ignore it.
  if (cluster.size() <= cs.cutoff)
  {
    //Creates the complete graph on the sequences that form the cluster
    g.connect_cluster(cluster);
  }
}

void compute_fingerprints(const DynArray<std::filesystem::path>& files, const std::vector<std::size_t>& repr,
                          const ClusterSettings& cs, DynArray<std::pair<Fingerprint, std::size_t>>& fingerprints) {
    DynArray<TwoArrayHash<uint32_t, char>> hashers(cs.fingerprint_size);
    for (std::size_t i = 0; i < hashers.size(); ++i)
    {
        hashers[i] = TwoArrayHash<uint32_t, char>(cs.shingle_size / 4, cs.shingle_size % 4);
    }
    ProfilerSteady::profile_thread("runround_sequential", [&] {
#pragma omp parallel for
        for (std::size_t i = 0; i < repr.size(); ++i)
        {
            ProfilerSteady::profile_thread("runround_" + std::to_string(omp_get_thread_num()), [&] {
                //std::cout << "Reading sequence " << cs.files[repr[i]].string() << std::endl;
                auto file_content = read_compacted_DNA(cs.files[repr[i]]);
                auto fingerprint = Fingerprint::min_hash_shingle(file_content, hashers);
                fingerprints[i] = std::make_pair(std::move(fingerprint), repr[i]);
            }, false);
        }
    }, false);
}

void compute_fingerprints_bottom_k(const DynArray<std::filesystem::path>& files, const std::vector<std::size_t>& repr,
                                   const ClusterSettings& cs, DynArray<std::pair<Fingerprint, std::size_t>>& fingerprints) {
    const TwoArrayHash<uint32_t, char> hash(cs.shingle_size >> 2, cs.shingle_size & 3); //Division by 4, modulo by 4
    ProfilerSteady::profile_thread("runround_sequential", [&] {
#pragma omp parallel for
        for (std::size_t i = 0; i < repr.size(); ++i) {
            ProfilerSteady::profile_thread("runround_" + std::to_string(omp_get_thread_num()), [&] {
                //std::cout << "Reading sequence " << cs.files[repr[i]].string() << std::endl;
                auto file_content = read_compacted_DNA(cs.files[repr[i]]);
                auto fingerprint = Fingerprint::bottom_k_hash(file_content, hash, cs.fingerprint_size);
                fingerprints[i] = std::make_pair(std::move(fingerprint), repr[i]);
            }, false);
        }
    }, false);
}

void run_round(const ClusterSettings &cs, const std::vector<std::size_t> &repr, ClusterGraph &g, DynArray<std::size_t> &collisions)
{
    DynArray<std::pair<Fingerprint, std::size_t>> fingerprints(repr.size());  //We're gonna check if we need the timing or not
    if (cs.bottom_k) {
        compute_fingerprints_bottom_k(cs.files, repr, cs, fingerprints);
    } else {
        compute_fingerprints(cs.files, repr, cs, fingerprints);
    }

    //We sort the fingerprints so that we have all the similar sequences in a contiguous chunk of the array.
    std::sort(fingerprints.begin(), fingerprints.end(), [](auto &f1, auto &f2)
    { return f1.first < f2.first; });
    //Are we allocating this every round?
    std::vector<std::size_t> cluster;
    cluster.reserve(fingerprints.size());
    cluster.push_back(fingerprints[0].second);
    //To print clusters
    std::map<std::size_t, std::size_t> cluster_sizes = std::map<std::size_t, std::size_t>();
    for (std::size_t i = 1; i < fingerprints.size(); ++i)
    {
        //If the fingerprints of sequence i-1 and sequence i are different, we are in a different
        //chunk of the array with a different cluster.
        if (fingerprints[i - 1].first != fingerprints[i].first)
        {
            handle_cluster(cs, cluster, g, collisions);
            if (!cluster_sizes.contains(cluster.size())) {
                cluster_sizes[cluster.size()] = 1;
            } else {
                cluster_sizes[cluster.size()] = cluster_sizes[cluster.size()] + 1;
            }
            //We clear the cluster array so that the next cluster can be filled and passed.
            //This is an inefficient way to do it but it works. (Why don't we just pass the region of the
            //fingerprints array? If we were working with raw arrays, we could just pass a pointer).
            cluster.clear();
        }
        cluster.push_back(fingerprints[i].second);
    }
    //handle the final cluster
    handle_cluster(cs, cluster, g, collisions);
    if (!cluster_sizes.contains(cluster.size())) {
        cluster_sizes[cluster.size()] = 1;
    } else {
        cluster_sizes[cluster.size()] = cluster_sizes[cluster.size()] + 1;
    }
    //Print clusters
    std::cout << "Clusters: ";
    for (auto map_it :cluster_sizes) {
        std::size_t size = map_it.first;
        std::size_t freq = map_it.second;
        std::cout << "Size " << size << ": " << freq << ", ";
    }
    std::cout << std::endl;
}

std::vector<std::size_t> representatives(ClusterGraph &g, const DynArray<std::size_t> &collisions)
{
  //An ordered map from sequence ID to a number.
  std::map<std::size_t, std::size_t> repr;
  //For each sequence...
  for (std::size_t i = 0; i < collisions.size(); ++i)
  {
    //We find the representative of the connected component in the union-find data structure
    auto cluster = g.find(i);
    //If this connected component is not on the map, we make the representative of the component
    //this sequence. If the number of collisions of the representative of the component is less
    //than the number of collisions of this sequence, make this sequence the representative.
    if (!repr.contains(cluster) || (collisions[repr[cluster]] < collisions[i]))
    {
      repr[cluster] = i;
    }
  }
  //Write all the representatives to the result vector.
  std::vector<std::size_t> result;
  result.reserve(g.number_of_components());
  for (const auto &[key, value] : repr)
  {
    result.push_back(value);
  }
  return result;
}

ClusterGraph *compute(ClusterSettings &cs)
{
  std::size_t original_threshold = cs.cutoff;
  std::size_t n = cs.files.size();
  std::size_t it = 1; //Iteration of the threshold
  std::cout << "Computing approximate graph. Total of " << cs.files.size() << " components." << std::endl;
  std::cout << "Initial threshold: " << cs.cutoff << std::endl;
  std::cout << "Current number of components: " << std::flush;
  ClusterGraph *g_ptr = new ClusterGraph(cs.files.size());
  DynArray<std::size_t> collisions(cs.files.size());
  std::vector<std::size_t> repr(cs.files.size());
  for (std::size_t i = 0; i < cs.files.size(); ++i)
  {
    collisions[i] = 0;
    repr[i] = i;
  }
  //We are going to keep track of how many connected components the graph had in the last round
  std::size_t n_comp_before = g_ptr->number_of_components();
  //Do first iteration
  for (std::size_t i = 0; i < cs.rounds_until_shrink; i++) {
      std::cout << g_ptr->number_of_components() << ", " << std::flush;
      run_round(cs, repr, (*g_ptr), collisions);
  }
  //This parameter controls how many rounds of adding edges do we do until we
  //replace the clusters by their representatives
  std::size_t until_shrink = 1;
  //Iterate
  while (!g_ptr->fully_connected())
  {
      //Check if we have the same number of representatives than before (rounds didn't achieve anything)
      //If so, strengthen the requisites for clustering
      std::size_t n_comp_now = g_ptr->number_of_components();
      if (n_comp_now == n_comp_before) {
          it++;
          cs.cutoff = std::floor(2*std::log2(n) * std::pow(it, 2));
          //cs.cutoff = cs.cutoff + (original_threshold / 2);
          std::cout << "New threshold: " << cs.cutoff << std::endl;
      }
      n_comp_before = n_comp_now;
    //Time to subsitute clusters by their representatives
    if (--until_shrink == 0)
    {
        repr = representatives((*g_ptr), collisions);
        until_shrink = cs.rounds_until_shrink;
    }
    //If there are less representatives left than the cutoff,
    //make cluster on them and finish
    if (repr.size() <= cs.cutoff)
    {
      handle_cluster(cs, repr, (*g_ptr), collisions);
      break;
    }

    //Otherwise, run a round of hashing and adding edges between equal fingerprints.
    std::cout << g_ptr->number_of_components() << ", " << std::flush;
    run_round(cs, repr, (*g_ptr), collisions);
  }
  std::cout << std::endl;
  return g_ptr;
}

void compute_add_weights(const DynArray<std::filesystem::path>& files, const AdjacencyList& adj_list, std::set<WeightedEdge>& result,
                         std::mutex& result_mutex, size_t from, bool parallel) {
    std::unique_ptr<Compressors::Compressor> selected_compressor = Compressors::getCompressor();
    selected_compressor->print_stats = false;
    const std::unordered_set<size_t>& target_set = adj_list.adjacent(from);
    size_t n_targets = target_set.size();
    const std::filesystem::path **target_paths = new const std::filesystem::path*[n_targets];
    size_t *id_tars = new size_t[n_targets];
    auto it = target_set.begin();
    for (size_t j = 0; j < n_targets; j++) {
        id_tars[j] = *it++;
        target_paths[j] = &files[id_tars[j]];
    }
    std::vector<size_t> weights = selected_compressor->compress_one_to_many_delete(files[from], target_paths, n_targets);
    if (parallel) {
        for (size_t j = 0; j < n_targets; j++) {
            result_mutex.lock();
            result.emplace(from, id_tars[j], weights[j]);
            result_mutex.unlock();
        }
    } else {
        for (size_t j = 0; j < n_targets; j++) {
            result.emplace(from, id_tars[j], weights[j]);
        }
    }
    delete[] target_paths;
    delete[] id_tars;
}

std::unique_ptr<std::set<WeightedEdge>> compute_rlz_for_edges(const AdjacencyList &adj_list, const DynArray<std::filesystem::path> &files, const DynArray<std::filesystem::path> &sa_files) {
#ifndef NDEBUG
    std::cerr << "Computing compression pairs. Total of " << adj_list.node_count() << " references." << std::endl;
    std::cerr << "Completed: " << std::flush;
    std::size_t completed_refs = 0;
#endif
    std::mutex result_mutex;
    std::unique_ptr<std::set<WeightedEdge>> result = std::make_unique<std::set<WeightedEdge>>();

    int max_threads = omp_get_max_threads();
    if (max_threads > 1) {
#pragma omp parallel for
        for (std::size_t from = 0; from < adj_list.node_count(); ++from) {
            ProfilerSteady::profile_thread("compute_edges_" + std::to_string(omp_get_thread_num()), [&] {
                compute_add_weights(files, adj_list, *result, result_mutex, from, true);
            }, false);
        }
    } else {
        for (std::size_t from = 0; from < adj_list.node_count(); ++from) {
            compute_add_weights(files, adj_list, *result, result_mutex, from, false);
        }
    }
    return result;
}

std::unique_ptr<std::set<WeightedEdge>> compute_approx_weights(const AdjacencyList &adj_list, const DynArray<std::filesystem::path> &files,
                                                               size_t shingle_size, size_t fingerprint_size) {
    std::mutex result_mutex;
    std::unique_ptr<std::set<WeightedEdge>> result = std::make_unique<std::set<WeightedEdge>>();
    size_t n = files.size();
    //Gotta run a round of fingerprinting
    const TwoArrayHash<uint32_t, char> hash(shingle_size >> 2, shingle_size & 3); //Division by 4, modulo by 4
    Fingerprint *fingerprints = new Fingerprint[n];
    int max_threads = omp_get_max_threads();
    if (max_threads > 1) {
#pragma omp parallel for
        for (size_t i = 0; i < n; i++) {
            ProfilerSteady::profile_thread("compute_edges_" + std::to_string(omp_get_thread_num()), [&] {
                auto file_content = read_compacted_DNA(files[i]);
                fingerprints[i] = Fingerprint::bottom_k_hash(file_content, hash, fingerprint_size);
            }, false);
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            auto file_content = read_compacted_DNA(files[i]);
            fingerprints[i] = Fingerprint::bottom_k_hash(file_content, hash, fingerprint_size);
        }
    }
    //Compute similarities
    size_t node_count = adj_list.node_count();
    if (max_threads > 1) {
#pragma omp for
        for (size_t from = 0; from < node_count; from++) {
            ProfilerSteady::profile_thread("compute_edges_" + std::to_string(omp_get_thread_num()), [&] {
                for (size_t to: adj_list.adjacent(from)) {
                    size_t similarity = fingerprints[from].ordered_intersection(fingerprints[to]);
                    float weight = 1 - (static_cast<float>(similarity) / (2 * fingerprint_size - similarity));
                    result_mutex.lock();
                    result->emplace(from, to, weight);
                    result_mutex.unlock();
                }
            }, false);
        }
    } else {
        for (size_t from = 0; from < node_count; from++) {
            for (size_t to : adj_list.adjacent(from)) {
                size_t similarity = fingerprints[from].ordered_intersection(fingerprints[to]);
                float weight = 1 - (static_cast<float>(similarity) / (2 * fingerprint_size - similarity));
                result->emplace(from, to, weight);
            }
        }
    }
    delete[] fingerprints;
    return result;
}

/**
* LEGACY CODE
 *
 *     /*
    if (Compressors::compressor_type == Compressors::RLZ) {
        #pragma omp parallel for
        for (std::size_t from = 0; from < adj_list.node_count(); ++from) {
            //Get the compressor that the user selected, and open the reference file
            //Note how we have changed it to eat the FASTA definition line too
            std::unique_ptr<RLZCompress> compressor = RLZCompress::from_dynamic_fasta(files[from], sa_files[from]);
            for (auto adj: adj_list.adjacent(from)) {
                FactorSink writer;
                //Note the FASTA change
                compressor->compress_fasta(files[adj].c_str(), writer);
                result_mutex.lock();
                result->emplace(from, adj, writer.factor_count());
                result_mutex.unlock();
            }
#ifdef DEBUG
            result_mutex.lock();
            completed_refs += 1;
            std::cerr << completed_refs << ", " << std::flush;
            result_mutex.unlock();
#endif
        }

    } else {
     */
