//
// Created by mpelo on 6/14/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_COMPRESSORHIRGCMAX_H
#define HIERARCHICALRELATIVELEMPELZIV_COMPRESSORHIRGCMAX_H

#include "hrlz/Compressors.hpp"
#include "hirgc/hirgc_max.hpp"
#include "file.hpp"

namespace Compressors {

    class CompressorHiRGCMax : public Compressor {
    public:

        CompressorHiRGCMax();
        ~CompressorHiRGCMax();

        /**
         * Methods inherited from Compressor
        */

        std::string get_name(const std::string& ref_filename, const std::string& target_filename) override;
        std::filesystem::path get_output_file(const std::string& ref_filename, const std::string& target_filename,
                                              const std::filesystem::path& output_dir) override;

        std::unique_ptr<RefDataStructure> getRefDataStructure(const std::filesystem::path& ref, bool decomp) override;
        std::unique_ptr<RefDataStructure> getRefDataStructure(std::istream& ref, bool decomp) override;

    };

}
#endif //HIERARCHICALRELATIVELEMPELZIV_COMPRESSORHIRGCMAX_H
