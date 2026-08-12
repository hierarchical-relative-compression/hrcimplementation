/**
 * Uses JSON to represent a function call, and the return from
 * such call. The format for a function call is, as of 25/04/2024:
 * {
 *  "fn_name": name of the function as a string
 *  "args": {
 *      "argc": number of arguments of the function
 *      "1": first argument
 *      "2": second argument
 *      ...
 *  }
 * }
 * 
 * This format allows us to have arguments of different types, since
 * C++ doesn't allow easily to have lists of different types.
*/

#include "json.hpp"
using json = nlohmann::json;
#include "json_parse.hpp"
#include "test_compress_hirgc_max.hpp"
#include "test_idocomp_max.hpp"
#include "iDoCompMax/matching_rlz.hpp"
#include "iDoCompMax/iDoCompMaxPostprocessing.hpp"
#include "test_agg_multimap.hpp"
#include "test_compress_rlz.hpp"
#include "test_finitestateentropy.hpp"

/**
 * Functions on this file are called in the following order:
*/
std::unique_ptr<std::string> json_parse(const char *message);
std::unique_ptr<std::string> function_call(json fn_obj);
std::string my_function(int a, int b);
std::unique_ptr<std::string> json_return(const std::string& message);

std::string my_function(int a, int b) {
    int res = a + b;
    std::string out = std::to_string(res);
    return out;
}

/**
 * PREREQUISITE: message is a valid JSON raw literal
 * Parses a JSON object that is understood as a function call,
 * and passes the object to function_call.
*/
std::unique_ptr<std::string> json_parse(const char *message) {
    json obj = json::parse(message);
    return function_call(obj);
}

/**
 * ADD HERE THE CUSTOM TEST FUNCTIONS
 *
 * Given a JSON object representing a function call, it decodes
 * the JSON and invokes the correct function. Passes the output of 
 * the function to json_return, and returns the JSON object representing
 * the return.
*/
std::unique_ptr<std::string> function_call(json fn_obj) {
    std::string fn_name = fn_obj["fn_name"].template get<std::string>();
    //To get the args object we would use:
    //std::unordered_map<std::string, json> args_obj = fn_obj["args"].template get<std::unordered_map<std::string, json>>();
    //But we don't really need that.
    if (fn_name == "my_function") {
        int a = fn_obj["args"]["0"].template get<int>();
        int b = fn_obj["args"]["1"].template get<int>();
        std::string ret = my_function(a, b);
        return json_return(ret);
//    } else if (fn_name == "test_idocomp_compress_decompress") {
//        std::string ref_fasta = fn_obj["args"]["0"].template get<std::string>();
//        std::string target_fasta = fn_obj["args"]["1"].template get<std::string>();
//        int64_t ret = test_idocomp_compress_decompress(ref_fasta, target_fasta);
//        return json_return(std::to_string(ret));
    } else if (fn_name == "test_rlz_compress_decompress") {
        std::string ref_fasta = fn_obj["args"]["0"].template get<std::string>();
        std::string target_fasta = fn_obj["args"]["1"].template get<std::string>();
        int64_t ret = test_rlz_compress_decompress(ref_fasta, target_fasta);
        return json_return(std::to_string(ret));
    } else if (fn_name == "test_compress_hirgcmax") {
        std::string ref_fasta = fn_obj["args"]["0"].template get<std::string>();
        std::string target_fasta = fn_obj["args"]["1"].template get<std::string>();
        uint64_t random = fn_obj["args"]["2"].template get<uint64_t>();
        int64_t ret = test_compress_hirgcmax(ref_fasta, target_fasta, random);
        return json_return(std::to_string(ret));
//    } else if (fn_name == "test_compress_sccg") {
//        std::string ref_fasta = fn_obj["args"]["0"].template get<std::string>();
//        std::string target_fasta = fn_obj["args"]["1"].template get<std::string>();
//        int64_t ret = test_compress_sccg(ref_fasta, target_fasta);
//        return json_return(std::to_string(ret));
//    } else if (fn_name == "test_compress_gdc2") {
//        std::string ref_fasta = fn_obj["args"]["0"].template get<std::string>();
//        std::string target_fasta = fn_obj["args"]["1"].template get<std::string>();
//        int64_t ret = test_compress_gdc2(ref_fasta, target_fasta);
//        return json_return(std::to_string(ret));
    } else if (fn_name == "test_idocomp_parse_rlz") {
        std::string ref_str = fn_obj["args"]["0"].template get<std::string>();
        std::string target_str = fn_obj["args"]["1"].template get<std::string>();
        std::vector<struct match> matches = test_compress_idocompmax(ref_str, target_str);
        json ret;
        ret["return"] = json::array();
        for (struct match factor: matches) {
            ret["return"].push_back({{"__match__", true},
                                     {"pos",       factor.pos},
                                     {"len",       factor.len},
                                     {"ch",        factor.mismatch}});
        }
        return std::make_unique<std::string>(ret.dump());
    } else if (fn_name == "test_idocompmax_parse_subs") {
        std::string ref_str = fn_obj["args"]["0"].template get<std::string>();
        std::string target_str = fn_obj["args"]["1"].template get<std::string>();
        std::vector<struct match> matches;
        std::vector<struct substitution> subs;
        int ret = test_idocompmax_parse_subs(ref_str, target_str, matches, subs);
        if (ret == -1) {
            return json_return("error");
        }
        json j;
        j["return"] = {{"matches", json::array()},
                       {"subs",    json::array()}};
        for (struct match factor: matches) {
            j["return"]["matches"].push_back({{"__match__", true},
                                              {"pos",       factor.pos},
                                              {"len",       factor.len},
                                              {"ch",        factor.mismatch}});
        }
        for (struct substitution sub: subs) {
            j["return"]["subs"].push_back({{"__sub__", true},
                                           {"pos",     sub.pos},
                                           {"ch",      sub.ch}});
        }
        return std::make_unique<std::string>(j.dump());
//    } else if (fn_name == "test_arith") {
//        std::string str = fn_obj["args"]["0"].template get<std::string>();
//        int64_t ret = compress_decompress_arith(str);
//        return json_return(std::to_string(ret));
    } else if (fn_name == "test_idocompmax_insertions_decomp") {
        std::string ref_fasta = fn_obj["args"]["0"].template get<std::string>();
        std::string target_fasta = fn_obj["args"]["1"].template get<std::string>();
        int64_t ret = test_idocompmax_insertions_decomp(ref_fasta, target_fasta);
        return json_return(std::to_string(ret));
//    } else if (fn_name == "test_partial_sums") {
//        std::vector<size_t> init = fn_obj["args"]["0"].template get<std::vector<size_t>>();
//        std::vector<std::string> instructions = fn_obj["args"]["1"].template get<std::vector<std::string>>();
//        std::vector<size_t> ret = test_partial_sums(init, instructions);
//        json ret_obj;
//        ret_obj["return"] = ret;
//        return std::make_unique<std::string>(ret_obj.dump());
    } else if (fn_name == "test_compress_decompress_idocompmax") {
        std::string ref_fasta = fn_obj["args"]["0"].template get<std::string>();
        std::string target_fasta = fn_obj["args"]["1"].template get<std::string>();
        int64_t ret = test_compress_decompress_idocompmax(ref_fasta, target_fasta);
        return json_return(std::to_string(ret));
    } else if (fn_name == "test_agg_multimap") {
        std::vector<size_t> elements = fn_obj["args"]["0"].template get<std::vector<size_t>>();
        uint64_t param = fn_obj["args"]["1"].template get<size_t>();
        int pos = test_agg_multimap(elements, param);
        json ret_obj;
        return json_return(std::to_string(pos));
    } else if (fn_name == "test_adj_list") {
        std::vector<size_t> elements = fn_obj["args"]["0"].template get<std::vector<size_t>>();
        int pos = test_adj_list(elements);
        json ret_obj;
        return json_return(std::to_string(pos));
    } else if (fn_name == "test_huffman") {
        std::string str = fn_obj["args"]["0"].template get<std::string>();
        int64_t ret = compress_decompress_huffman(str);
        return json_return(std::to_string(ret));
    } else if (fn_name == "test_huffman_api") {
        std::string str = fn_obj["args"]["0"].template get<std::string>();
        int64_t ret = compress_decompress_huffman_api(str);
        return json_return(std::to_string(ret));
    } else if (fn_name == "test_fse_mem") {
        std::string str = fn_obj["args"]["0"].template get<std::string>();
        int64_t ret = compress_decompress_fse_mem(str);
        return json_return(std::to_string(ret));
    } else {
        //Not recognized function
        return std::make_unique<std::string>("Function not recognized: " + fn_name);
    }
}

/**
 * Creates a JSON object representing the return of a function.
*/
std::unique_ptr<std::string> json_return(const std::string& message) {
    json ret;
    ret["return"] = message;
    return std::make_unique<std::string>(ret.dump());
}
