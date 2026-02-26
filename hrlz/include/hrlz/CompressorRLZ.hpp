//
// Created by mpelo on 10/6/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_COMPRESSORRLZ_HPP
#define HIERARCHICALRELATIVELEMPELZIV_COMPRESSORRLZ_HPP

#include "hrlz/Compressors.hpp"
#include "rlz/rlz.hpp"
#include <memory>

namespace Compressors {
    class CompressorRLZ : public Compressor {
    private:
        std::unique_ptr<RLZCompress> rlz_compressor;
    public:

        CompressorRLZ();
        ~CompressorRLZ();

        /**
         * Methods inherited from Compressor
        */
        size_t compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                    const std::filesystem::path& ref_sa) override;
        size_t compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                  const std::filesystem::path& ref_sa) override;

        //Private methods
        std::string get_filename(const std::filesystem::path& ref, const std::filesystem::path& target);
        static void decompress_pair(const DynArray<char>& seq_ref, const std::string& tar_comp_filename,
                                    const std::string& tar_original_filename,
                                    const std::filesystem::path& output_dir, const std::filesystem::path& input_directory);

        void decompress_pair(const std::filesystem::path& ref, const std::filesystem::path& target, const std::filesystem::path& output) override;
        std::unique_ptr<RefDataStructure> getRefDataStructure(const std::filesystem::path& ref, bool decomp) override;
        std::unique_ptr<RefDataStructure> getRefDataStructure(std::istream& ref, bool decomp) override;
    };
}

#endif //HIERARCHICALRELATIVELEMPELZIV_COMPRESSORRLZ_HPP
