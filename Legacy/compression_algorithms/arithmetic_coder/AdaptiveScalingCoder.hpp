#pragma once
#include "ArithmeticCoder.hpp"
#include "Statistics.hpp"
#include "AdaptiveModel.hpp"
#include "BitUtils/BitWriter.hpp"
#include "BitUtils/BitReader.hpp"
#include "FastAdaptiveModel.hpp"
#include <sstream>

#include <string>

class AdaptiveScalingCoder : public ArithmeticCoder
{
public:
	AdaptiveScalingCoder(bool printProgress = false);
	Statistics encode(std::string& str, const std::filesystem::path& out, bool start=true, bool end=true) override;
    Statistics encode(std::istream& ss, size_t n, const std::filesystem::path& out, bool start=true, bool end=true) override;
	void decode(const std::filesystem::path& path_in, std::stringstream& out, bool start=true, bool end=true) override;

private:
    static constexpr uint64_t powerOf(uint64_t a, uint64_t n) {
        return n == 0 ? 1 : a * powerOf(a, n - 1);
    }

    /* Parameters for encoding algorithm */
    static constexpr uint64_t PRECISION = 32;
    static constexpr uint64_t WHOLE = 1ULL << PRECISION;
    static constexpr uint64_t HALF = WHOLE / 2;
    static constexpr uint64_t QUARTER = WHOLE / 4;

    /* Parameters for data model */
    static constexpr int MODEL_SIZE = 256 + 1;					// 256 + 1 for EOF symbol
    static constexpr int MODEL_EOF_SYMBOL = 256;				// last symbol in model is EOF symbol
    static constexpr int MODEL_MAX_FREQUENCY = QUARTER - 1;		// max total frequency for model

	bool printProgress;
	int currentProgress = 0;
	const size_t progressbarWidth = 70;
    uint64_t bytesRead {};
    uint64_t bits_written {};
    uint64_t a {};
    uint64_t b {};
    int s {};
    BitWriter out;
    AdaptiveModel model;
    //for decoding
    BitReader in {};
    FastAdaptiveModel model_dec { MODEL_SIZE, MODEL_MAX_FREQUENCY };
    long filesize {};
    uint64_t bitsRead {};
    uint64_t z {};

	inline void updateProgress(double progress);
	inline void clearProgress();

    void loadModel(const std::filesystem::path& in_model, AdaptiveModel& model, BitWriter& bw);
    void storeModel(const std::filesystem::path& out_model, AdaptiveModel& model, BitWriter& bw);
    std::filesystem::path model_path;
};
