//
// Created by mpelo on 12/28/25.
//

#ifndef CSV_CSV_INTERFACE_HPP
#define CSV_CSV_INTERFACE_HPP

#include <vector>
#include <filesystem>

class CSV_file {
private:
    std::vector<std::vector<std::string>> data;
    std::filesystem::path path;
    size_t max_cols;
public:
    const char separator;

    CSV_file(const std::filesystem::path& file);
    CSV_file(const std::filesystem::path& file, char separator);
    ~CSV_file();

    void flush();

    void write_cell(size_t row, size_t col, const std::string& cell);
    std::string read_cell(size_t row, size_t col) const;
};

#endif //CSV_CSV_INTERFACE_HPP
