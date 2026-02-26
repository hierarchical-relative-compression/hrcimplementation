//
// Copyright (c) 2020 Sebastian Fojcik
// Copyright (c) 2024 Máximo Pérez López
//

#pragma once

#include <stdexcept>
#include <vector>
#include "PartialSums.hpp"

class FastAdaptiveModel
{
public:
	FastAdaptiveModel(size_t numberOfSymbols, size_t maxTotalFrequency) :
            //We initialize to 1 all frequencies. If the max is too small, error (don't initialize)
            F(numberOfSymbols >= maxTotalFrequency ?
              throw std::invalid_argument("max frequency is too low to fit that number of symbols") :
              1uL, numberOfSymbols),
            n(numberOfSymbols), totalFrequencyCounter(numberOfSymbols), MAX_TOTAL_FREQUENCY(maxTotalFrequency)
	{ }

	void update(size_t symbol)
	{
		if (totalFrequencyCounter >= MAX_TOTAL_FREQUENCY) {
            // Check if frequency counter reaches max. If so, halve all frequencies, but keep them positive (at least 1).
            // This is done by retrieving the original frequencies, doing the halving, and rebuilding the Fenwick tree.
            totalFrequencyCounter = 0;
            std::vector<size_t> frequencies = F.retrieve_array();
            for (auto& freq : frequencies) {
				freq = freq / 2 != 0 ? freq / 2 : 1;			
				totalFrequencyCounter += freq;
			}
            frequencies.at(symbol)++;
            totalFrequencyCounter++;
            F.reset(frequencies);
		} else {
            //We just do a +1 on the symbol frequency
            F.update(symbol, 1);
            totalFrequencyCounter++;
        }
	}

	size_t frequencyBegin(size_t symbol)
	{
        //The previous partial sum of the frequency array
        if (symbol == 0) {
            return 0;
        } else {
            return F.query(symbol - 1);
        }
	}

	size_t frequencyEnd(size_t symbol)
	{
        //The partial sum of the frequency array
		return F.query(symbol);
	}

	size_t totalFrequency()
	{
		return totalFrequencyCounter;
	}

	size_t size()
	{
		return n;
	}

private:
    FenwickTree F;
    size_t n;

	size_t totalFrequencyCounter;	// total number of symbols appearance.
	const size_t MAX_TOTAL_FREQUENCY;
};