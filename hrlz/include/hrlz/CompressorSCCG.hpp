#ifndef SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORSCCG_HPP_
#define SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORSCCG_HPP_

#include "hrlz/Compressors.hpp"
#include "file.hpp"

namespace Compressors {

    extern bool java_open;

    class CompressorSCCG : public Compressor {
    public:

        CompressorSCCG();
        ~CompressorSCCG();

        /**
         * Methods inherited from Compressor
        */
        size_t compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                    const std::filesystem::path& ref_sa) override;
        size_t compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                  const std::filesystem::path& ref_sa) override;

        size_t compress_dataset(const DynArray<std::filesystem::path>& files,
                                const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id,
                                const DynArray<std::filesystem::path>& sa_files);


        void decompress_dataset(const std::filesystem::path& input_directory);

        /**
         * Other methods of the implementation
        */
        static void check_java();
        static void end_java();

        void decompress_pair(const std::filesystem::path& ref_filename, const std::filesystem::path& target_filename);

        size_t compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse);
        size_t compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse);
        size_t compress_hrlz(const DynArray<std::filesystem::path>& files,
                             const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id);
    };

}

#endif /* SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORSCCG_HPP_ */