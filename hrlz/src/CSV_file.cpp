//
// Created by mpelo on 12/28/25.
//

#include "hrlz/csv_interface.hpp"

#include <fstream>

CSV_file::~CSV_file() {}

CSV_file::CSV_file(const std::filesystem::path &path) : CSV_file(path, ';') {}

CSV_file::CSV_file(const std::filesystem::path& path_to_file, char separator) : path(path_to_file), separator(separator) {
    max_cols = 0;
    if (!std::filesystem::exists(path_to_file)) {
        return;
    }
    std::ifstream file(path, std::ios_base::in);
    if (!file)
        throw std::runtime_error("Couldn't open " + path.string());
    //Read file line by line
    std::string line;
    while (std::getline(file, line)) {
        data.push_back(std::vector<std::string>());
        std::vector<std::string>& row = data.back();
        //Split by separator
        size_t last = 0, next = 0;
        while ((next = line.find(separator, last)) != std::string::npos) {
            row.push_back(line.substr(last, next - last));
            last = next + 1;
        }
        row.push_back(line.substr(last));
        max_cols = std::max(max_cols, row.size());
    }
}


void CSV_file::flush() {
    std::ofstream file(path, std::ios_base::out);
    if (!file)
        throw std::runtime_error("Couldn't open " + path.string() + " for writing");
    size_t rows = data.size();
    char newline = '\n';
    for (size_t i = 0; i < rows; i++) {
        std::vector<std::string>& row = data[i];
        size_t cols = row.size();
        if (cols > 0)
            file.write(row[0].c_str(), row[0].size());
        size_t j;
        for (j = 1; j < cols; j++) {
            file.write(&separator, 1);
            file.write(row[j].c_str(), row[j].size());
        }
        for (j = std::max(1UL, cols); j < max_cols; j++) {
            file.write(&separator, 1);
        }
        file.write(&newline, 1);
    }
}

void CSV_file::write_cell(size_t row, size_t col, const std::string& cell) {
    size_t rows = data.size();
    if (row >= rows) {
        for (size_t i = rows; i <= row; i++) {
            data.push_back(std::vector<std::string>());
        }
    }
    std::vector<std::string>& datarow = data[row];
    size_t cols = datarow.size();
    if (col >= cols) {
        for (size_t i = cols; i <= col; i++) {
            datarow.push_back("");
        }
        max_cols = std::max(max_cols, datarow.size());
    }
    data[row][col] = cell;
}

std::string CSV_file::read_cell(size_t row, size_t col) const {
    if (row >= data.size()) {
        return "";
    }
    const std::vector<std::string>& datarow = data[row];
    if (col >= datarow.size()) {
        return "";
    }
    return datarow[col];
}
