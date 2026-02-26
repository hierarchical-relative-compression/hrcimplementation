//
// Created by mpelo on 8/10/24.
//
#include "hrlz/CompressorIDoCompMax.hpp"
#include "hrlz/file.hpp"
#include "iDoCompMax/iDoCompMaxMain.hpp"
#include <queue>
#include "common/SteadyTimer.hpp"
#include <omp.h>
#include "HuffmanFile.hpp"

namespace Compressors {

    CompressorIDoCompMax::CompressorIDoCompMax() : Compressor(new HuffmanEncoder()) {
        dynamic_cast<HuffmanEncoder*>(encoder)->set_idocomp();
    }
    CompressorIDoCompMax::~CompressorIDoCompMax() {
        delete encoder;
    }

    std::unique_ptr<RefDataStructure> CompressorIDoCompMax::getRefDataStructure(std::istream &ref, bool decomp) {
        std::unique_ptr<iDoCompMaxRef> refds = std::make_unique<iDoCompMaxRef>(ref, decomp);
        return refds;
        //return reinterpret_cast<std::unique_ptr<RefDataStructure> &&>(refds);
    }

    std::unique_ptr<RefDataStructure> CompressorIDoCompMax::getRefDataStructure(const std::filesystem::path& ref, bool decomp) {
        return std::make_unique<iDoCompMaxRef>(ref, decomp);
    }
}