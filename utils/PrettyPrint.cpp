//
// Created by mpelo on 8/19/24.
//

#include "include/utils/PrettyPrint.hpp"

/**
 * Prints to the stream the size in bytes, rounded to 3 significative digits.
 * @param bytes the size in bytes
 */
std::string format_size(size_t bytes) {
    std::ostringstream out;
    out << std::setprecision(3);
    if (bytes >= (1L << 30)) { //1 GB
        double gbs = static_cast<double>(bytes) / static_cast<double>(1L << 30);
        out << gbs << " GB";
    } else if (bytes >= (1L << 20)) { //MB
        double mbs = static_cast<double>(bytes) / static_cast<double>(1L << 20);
        out << mbs << " MB";
    } else if (bytes >= (1L << 10)) { //KB
        double kbs = static_cast<double>(bytes) / static_cast<double>(1L << 10);
        out << kbs << " KB";
    } else {
        out << bytes << " B";
    }
    return out.rdbuf()->str();
}

std::string format_time(size_t ns) {
    std::ostringstream out;
    out << std::setprecision(3);
    unsigned long long s = 1000000000;
    if (ns >= (s * 3600)) { //hours
        double hours = static_cast<double>(ns) / (s * 3600);
        out << hours << " h";
    } else if (ns >= (s * 60)) {
        double min = static_cast<double>(ns) / (s * 60);
        out << min << " min";
    } else if (ns >= s) {
        double sec = static_cast<double>(ns) / s;
        out << sec << " s";
    } else if (ns >= 1000000) {
        double ms = static_cast<double>(ns) / 1000000;
        out << ms << " ms";
    } else if (ns >= 1000) {
        double us = static_cast<double>(ns) / 1000;
        out << us << " us";
    } else {
        out << ns << " ns";
    }
    return out.rdbuf()->str();
}

void print_header(std::ostream& out, const std::vector<std::string>& headers, const std::vector<alignment>& alignment) {
    //Print in markdown format:
    // | header1 | header2 ... header_n |
    // | -- | -- ... -- |
    size_t size = headers.size();
    if (size == 0) { std::cerr << "no headers to print"; return; }
    for (size_t i = 0; i < size; i++) {
        out << "| " << headers[i] << " ";
    }
    out << "|" << std::endl;

    for (size_t i = 0; i < size; i++) {
        out << "| ";
        if (alignment[i] == LEFT) {
            out << ":-- ";
        } else if (alignment[i] == CENTER) {
            out << ":--: ";
        } else {
            out << " --: ";
        }
    }
    out << "|" << std::endl;
}

void print_row(std::ostream& out, const std::vector<std::string>& cells) {
    //Print in markdown format: | cell | cell ... cell |
    size_t size = cells.size();
    if (size == 0) { std::cerr << "No cells to print" << std::endl; return;}
    for (size_t i = 0; i < size; i++) {
        out << "| " << cells[i];
    }
    out << "|" << std::endl;
}