//
// Created by mpelo on 8/6/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_IDOCOMPMAXMAIN_HPP
#define HIERARCHICALRELATIVELEMPELZIV_IDOCOMPMAXMAIN_HPP

#include <filesystem>
#include "RefDataStructure.hpp"
#include "Encoder.hpp"

extern enum IDC_Encoding {
    ARITH, PPM, HUFF
} iDoCompMaxEncoding;

char *read_fasta(const std::filesystem::path& path, char **fasta_line, size_t *n, size_t *n_fasta_line);
int *read_sa(const std::filesystem::path& path, size_t ref_size);

int iDoCompMaxMain(const std::filesystem::path& ref, const std::filesystem::path& ref_SA, const std::filesystem::path& target,
                   const std::filesystem::path& output_file, bool print_stats);
int iDoCompMaxDecompress(const std::filesystem::path& ref, const std::filesystem::path& target_input,
                         const std::filesystem::path& output_file);

//Class to hold the data of a parsed reference
class iDoCompMaxRef : public RefDataStructure {
    //We hold the reference string, raw, and its suffix array, raw too
    char *ref;
    int *ref_sa;
    size_t n_ref;
    //We don't even use the fasta line for the reference
public:
    //Constructor
    iDoCompMaxRef(const std::filesystem::path& ref_path, bool decomp);
    iDoCompMaxRef(std::istream& in, bool decomp);
    //Destructor
    ~iDoCompMaxRef();
    //Compress (returns 0 if correct, otherwise there's been an error)
    void parseRef(std::istream& in) override;
    void parseRef(std::istream& in, bool print_stats);
    void compress_target(const std::filesystem::path& in, std::ostream& out) const override;
    void decompress_target(std::istream& in, std::ostream& out) const override;
    static std::string get_id_ref(const std::filesystem::path& ref);
    std::string get_id();

    int compress(const std::filesystem::path &target_path, const std::filesystem::path &output_file, bool print_stats);

private:
    //Some useful functions
    int compress_to_disk(char *target, char *target_fasta_line, size_t n_target, size_t n_target_fasta_line,
                         std::ostream& output_file) const;
};


#endif //HIERARCHICALRELATIVELEMPELZIV_IDOCOMPMAXMAIN_HPP
