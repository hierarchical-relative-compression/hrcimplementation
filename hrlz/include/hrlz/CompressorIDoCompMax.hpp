//
// Created by mpelo on 8/10/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_COMPRESSORIDOCOMPMAX_HPP
#define HIERARCHICALRELATIVELEMPELZIV_COMPRESSORIDOCOMPMAX_HPP

#include "Compressors.hpp"
#include "iDoCompMax/iDoCompMaxMain.hpp"

namespace Compressors {

    class CompressorIDoCompMax : public Compressor {
    public:
        CompressorIDoCompMax();
        ~CompressorIDoCompMax();

        std::unique_ptr<RefDataStructure> getRefDataStructure(const std::filesystem::path& ref, bool decomp) override;
        std::unique_ptr<RefDataStructure> getRefDataStructure(std::istream& ref, bool decomp) override;
    };

}

#endif //HIERARCHICALRELATIVELEMPELZIV_COMPRESSORIDOCOMPMAX_HPP
