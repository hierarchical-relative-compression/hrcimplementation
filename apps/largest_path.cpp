//
// Created by mpelo on 1/5/26.
//

#include <iostream>
#include "../hrlz/include/hrlz/file.hpp"
#include <queue>

size_t longest(std::vector<size_t> *arbor, size_t current_node) {
    const std::vector<size_t>& children = arbor[current_node];
    if (children.size() == 0) {
        return 0;
    } else {
        size_t max_len = 1;
        for (size_t child : children) {
            max_len = std::max(max_len, 1 + longest(arbor, child));
        }
        return max_len;
    }
}

size_t total_length(std::vector<size_t> *arbor, size_t current_node, size_t& n_leaves) {
    const std::vector<size_t>& children = arbor[current_node];
    if (children.size() == 0) {
        n_leaves = 1;
        return 0;
    } else {
        size_t total_len = 0;
        size_t leaves = 0;
        for (size_t child : children) {
            size_t child_leaves;
            total_len += total_length(arbor, child, child_leaves);
            total_len += child_leaves;
            leaves += child_leaves;
        }
        n_leaves = leaves;
        return total_len;
    }
} //Seems to be incorrect

size_t brute_force_total_length(std::unique_ptr<FullTreeMetadata>& met) {
    size_t total_length = 0;
    size_t n = met->n_sequences;
    size_t *parents = met->parents.get();
    size_t root = met->root_id;
    for (size_t i = 0; i < n; i++) {
        //Find length of path
        size_t len = 0;
        size_t leaf = i;
        while (leaf != root) {
            leaf = parents[leaf];
            len++;
        }
        total_length += len;
    }
    return total_length;
}

void print_usage() {
    const char *usage = R"(Usage:

largest_path metadata

Finds the longest path from a root to a leaf.)";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        std::exit(1);
    }
    std::filesystem::path path = argv[1];
    std::unique_ptr<FullTreeMetadata> met = read_metadata_full_tree(path);
    //DFS
    std::vector<size_t> *arbor = met->arbor.get();
    size_t root_id = met->root_id;
    size_t longest_path = longest(arbor, root_id);
    size_t root_leaves;
    size_t total_len = total_length(arbor, root_id, root_leaves);
    size_t total_len_brute_force = brute_force_total_length(met);
    std::cout << "Length of the longest path: " << longest_path << std::endl;
    std::cout << "Average length: " << static_cast<double>(total_len) / met->n_sequences << std::endl;
    std::cout << "Average length by brute force: " << static_cast<double>(total_len_brute_force) / met->n_sequences << std::endl;
}