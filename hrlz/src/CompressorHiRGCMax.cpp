//
// Created by mpelo on 6/14/24.
//
#include "hrlz/CompressorHiRGCMax.hpp"

#include "HuffmanFile.hpp"

#include "hrlz/Compressors.hpp"
#include "hirgc/hirgc_max.hpp"
#include <queue>
#include <stdexcept>
#include "common/SteadyTimer.hpp"
#include <omp.h>

namespace Compressors {
    using namespace hirgc_mrmcmax;

    CompressorHiRGCMax::CompressorHiRGCMax() : Compressor(new HuffmanEncoder()) {}

    CompressorHiRGCMax::~CompressorHiRGCMax() {
        delete encoder;
    }

    std::string CompressorHiRGCMax::get_name(const std::string& ref_filename, const std::string& target_filename) {
        return target_filename + "_ref_" + ref_filename;
    }
    std::filesystem::path CompressorHiRGCMax::get_output_file(const std::string& ref_filename, const std::string& target_filename,
                                          const std::filesystem::path& output_dir) {
        return output_dir / (get_name(ref_filename, target_filename));
    }

    std::unique_ptr<RefDataStructure> CompressorHiRGCMax::getRefDataStructure(const std::filesystem::path& ref, bool decomp) {
        std::unique_ptr<HiRGCRef> refDS = std::make_unique<HiRGCRef>(ref, decomp);
        return refDS;
    }
    std::unique_ptr<RefDataStructure> CompressorHiRGCMax::getRefDataStructure(std::istream& ref, bool decomp) {
        std::unique_ptr<HiRGCRef> refDS = std::make_unique<HiRGCRef>(ref, decomp);
        return refDS;
    }
}