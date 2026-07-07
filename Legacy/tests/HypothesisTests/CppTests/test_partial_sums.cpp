//
// Created by mpelo on 10/30/24.
//

#include "HRLZTests.h"

#include "test_partial_sums.hpp"
#include "../../../data_structures/PartialSums.hpp"
#include <iostream>
#include <sstream>

#include "../../../../tests/HypothesisTests/CppEngine/json.hpp"
using json = nlohmann::json;

char *test_partial_sums(char *json_str, uint64_t n_json) {
    json obj = json::parse(std::string(json_str, n_json));
    std::vector<size_t> init = obj["args"]["0"].template get<std::vector<size_t>>();
    std::vector<std::string> instructions = obj["args"]["1"].template get<std::vector<std::string>>();
    std::vector<size_t> ret = test_partial_sums(init, instructions);
    json ret_obj;
    ret_obj["return"] = ret;
    return make_C_string(ret_obj.dump());
}


int execute_instruction(PartialSums& ps, std::vector<size_t>& queries, std::string instruction) {
    std::istringstream iss(instruction);
    char type;
    iss >> type;
    if (type == 'q') {
        char parenthesis; size_t index;
        iss >> parenthesis;
        if (parenthesis != '(') { std::cerr << "Unknown format: " << instruction << ", aborting" << std::endl; return -1; }
        iss >> index;
        queries.push_back(ps.query(index));
    } else if (type == 'u') {
        char parenthesis, comma;
        size_t pos; int64_t x;
        iss >> parenthesis;
        if (parenthesis != '(') { std::cerr << "Unknown format: " << instruction << ", aborting" << std::endl; return -1; }
        iss >> pos;
        iss >> comma;
        if (comma != ',') { std::cerr << "Unknown format: " << instruction << ", aborting" << std::endl; return -1; }
        iss >> x;
        ps.update(pos, x);
    } else {
        //Unknown instruction!!!
        std::cerr << "Unknown instruction testing partial sums: " << instruction << ", aborting" << std::endl;
        return -1;
    }
    return 0;
}

std::vector<size_t> test_partial_sums(const std::vector<size_t>& init, const std::vector<std::string>& instructions) {
    std::vector<size_t> res;
    FenwickTree ft(init);
    for (const std::string& i : instructions) {
        int check = execute_instruction(ft, res, i);
        if (check != 0) {
            return std::vector<size_t>();
        }
    }
    return res;
}