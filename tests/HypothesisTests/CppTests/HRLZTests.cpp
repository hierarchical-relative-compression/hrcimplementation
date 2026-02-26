//
// Created by mpelo on 2/3/25.
//

#include "HRLZTests.h"

#include "../CppEngine/json.hpp"
using json = nlohmann::json;
#include <iostream>

void free_ptr(char *pointer) {
    std::cout << "Freeing pointer: " << static_cast<void*>(pointer) << std::endl;
    free(pointer);
}

char *make_C_string(const std::string& str) {
    char *c_str = static_cast<char *>(malloc(str.size() + 1));
    str.copy(c_str, str.size());
    c_str[str.size()] = '\0';
    return c_str;
}

char *json_return_C(const std::string& message) {
    json ret;
    ret["return"] = message;
    std::string str_ret = ret.dump();
    return make_C_string(str_ret);
}