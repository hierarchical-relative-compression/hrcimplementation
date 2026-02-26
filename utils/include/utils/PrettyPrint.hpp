/**
 * Created by mpelo on 8/19/24.
 * To pretty print a table to a Markdown format (.md)
*/

#ifndef HIERARCHICALRELATIVELEMPELZIV_PRETTYPRINT_HPP
#define HIERARCHICALRELATIVELEMPELZIV_PRETTYPRINT_HPP

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>

enum alignment {
    LEFT, CENTER, RIGHT
};

/**
 * Prints to the stream the size in bytes, rounded to 3 significative digits.
 * @param bytes the size in bytes
 */
std::string format_size(size_t bytes);

std::string format_time(size_t ns);

void print_header(std::ostream& out, const std::vector<std::string>& headers, const std::vector<alignment>& alignment);

void print_row(std::ostream& out, const std::vector<std::string>& cells);

#endif //HIERARCHICALRELATIVELEMPELZIV_PRETTYPRINT_HPP
