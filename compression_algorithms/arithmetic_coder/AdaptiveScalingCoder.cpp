//
// Copyright (c) 2020 Sebastian Fojcik
//

#include "AdaptiveScalingCoder.hpp"
#include "AdaptiveModel.hpp"
#include "FastAdaptiveModel.hpp"
#include "Statistics.hpp"
#include "BitUtils/BitWriter.hpp"
#include "BitUtils/BitReader.hpp"
#include "FileSize.hpp"

#include <cstdint>
#include <cmath>
#include <fstream>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <filesystem>

constexpr uint64_t powerOf(uint64_t a, uint64_t n)
{
	return n == 0 ? 1 : a * powerOf(a, n - 1);
}

using byte_t = uint8_t;

AdaptiveScalingCoder::AdaptiveScalingCoder(bool printProgress)
    : printProgress(printProgress), model(MODEL_SIZE, MODEL_MAX_FREQUENCY) {
}

Statistics AdaptiveScalingCoder::encode(std::string& s, const std::filesystem::path& path_out, bool start, bool end) {
    size_t n = s.size();
    std::stringstream ss = std::stringstream(std::move(s));
    return encode(ss, n, path_out, start, end);
}

Statistics AdaptiveScalingCoder::encode(std::istream& in, size_t n, const std::filesystem::path& path_out, bool start, bool end)
{
    //String stream
    if (start) {
        out.open(path_out.string());
    } else {
        loadModel(model_path, model, out);
    }

	// for progress bar
	long filesize = n;
	bytesRead = 0;
    bits_written = 0;

	a = 0L;
	b = WHOLE;
	s = 0L;

	bool endOfInput = false;
	while (!endOfInput)
	{
		int byte = in.get();
		out.beginByte(byte);		// for statistics purposes
		if (byte == EOF) {
            endOfInput = true;
            byte = MODEL_EOF_SYMBOL; // get representation of EOF symbol
		}

		size_t symbol = byte;
		uint64_t w = b - a;
		b = a + llround(w * ((double)model.frequencyEnd(symbol) / model.totalFrequency()));
		a = a + llround(w * ((double)model.frequencyBegin(symbol) / model.totalFrequency()));

		// Scaling
		while (true)
		{
			if (b < HALF) {			// Expand left. [a = 2a, b = 2b]
				out.write(0);
				out.writeN(1, s);
                bits_written += s + 1;
				s = 0;
			}
			else if (a > HALF) {	// Expand right. [a = 2(a-half), b = 2(b-HALF)]
				out.write(1);
				out.writeN(0, s);
                bits_written += s + 1;
				s = 0;
				a -= HALF;
				b -= HALF;
			}
			else if (a > QUARTER && b < 3 * QUARTER) {	// Expand middle (blow up). [a = 2(a-quarter), b = 2(b-quarter)]
				s += 1;
                bits_written++;
				a -= QUARTER;
				b -= QUARTER;
			}
			else {		// No more scaling.
				break;	// At this point [a,b] range is at least HALF in length.
			}
			a *= 2;
			b *= 2;
		}
		
		// update progress bar
		bytesRead++;
        if (printProgress) {
            updateProgress((double)(bytesRead) / filesize);
        }

		model.update(symbol);
        //If we have written more than the size of the file, we haven't compressed anything!
        //(we are checking after scaling. It could happen that many bytes are written in scaling,
        //but we are going to assume that the scaling in itself won't blow up the compression)
        //We are not going to implement this functionality yet
        /*
        if (bits_written > filesize * 8) {
            throw std::runtime_error("Wrote " + std::to_string(bits_written * 8) + " bytes for a file of " + std::to_string(filesize) + " bytes!!! Compression failed");
        }
         */
	}
    if (end) {
        s += 1;
        if (a <= QUARTER) {
            out.write(0);
            out.writeN(1, s);
        }
        else {
            out.write(1);
            out.writeN(0, s);
        }
        out.flush();
    } else {
        storeModel(model_path, model, out);
    }
	clearProgress();
	return Statistics(out.getStats());
}

void AdaptiveScalingCoder::decode(const std::filesystem::path& path_in, std::stringstream& out, bool start, bool end)
{
    if (start) {
        in.open(path_in);	// for progress bar
        filesize = getFileSize(path_in);
        bitsRead = 0;

        a = 0L;
        b = WHOLE;
        z = 0L;

        size_t i = 1;
        while (i <= PRECISION && !in.eof()) {		// Initialize 'z' with as many bits as you can
            if (in.read())	// bit '1' read
                z += powerOf(2, PRECISION - i);
            i += 1;
            bitsRead++;  // for progress bar
        }
    }
    bool stop = false;
	while (!stop)
	{
		// decode a symbol (binary search)
		size_t left = 0;
		size_t right = model_dec.size() - 1;
		while (left <= right && !stop)
		{
			size_t symbol = (left + right) / 2;
			uint64_t w = b - a;
			uint64_t b0 = a + llround(w * ((double)model_dec.frequencyEnd(symbol) / model_dec.totalFrequency()));
			uint64_t a0 = a + llround(w * ((double)model_dec.frequencyBegin(symbol) / model_dec.totalFrequency()));

			assert(a0 < b0); // must be true

			if (z < a0)
				right = symbol - 1;
			else if (z >= b0)
				left = symbol + 1;
			else // symbol found: a0 <= z < b0
			{
				if (symbol == MODEL_EOF_SYMBOL) { // End Of File symbol
                    if (end) {
                        clearProgress();
                        return;
                    } else {
                        stop = true;
                    }
				}

				byte_t decoded = (byte_t)symbol;
                if (symbol != MODEL_EOF_SYMBOL)
				    out.put(decoded);

				a = a0;
				b = b0;

				model_dec.update(decoded);
				updateProgress((double)(bitsRead + 16) / filesize / 8);
				break;
			}
		}

		// Scaling
		while (true) 
		{
			if (b < HALF) {				// Expand left
				/* nothing */
			}
			else if (a > HALF) {		// Expand right
				a -= HALF;
				b -= HALF;
				z -= HALF;
			}
			else if (a > QUARTER && b < 3 * QUARTER) {	// Expand middle (blow up)
				a -= QUARTER;
				b -= QUARTER;
				z -= QUARTER;
			}
			else {		// No more scaling.
				break;	// At this point [a,b] range is at least HALF in length.
			}
			a *= 2;
			b *= 2;
			z *= 2;

			// Update z approximation
			if (!in.eof() && in.read())
				z += 1;
			//i += 1;
			bitsRead++;  // for progress bar
		}
	}
}

void AdaptiveScalingCoder::updateProgress(double progress)
{
	if (!printProgress || int(progress * 100.0) == currentProgress)
		return;

	currentProgress = std::min(int(progress * 100.0), 100);

	std::cout << "[";
	int pos = (int)((double)progressbarWidth * progress);
	for (int i = 0; i < progressbarWidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] " << currentProgress << " %\r";
	std::cout.flush();
}

void AdaptiveScalingCoder::clearProgress()
{
	if (printProgress)
	{
		std::cout << std::string(progressbarWidth + 8, ' ');
		std::cout << "\r";
		std::cout.flush();
	}
}


void AdaptiveScalingCoder::loadModel(const std::filesystem::path& in_model, AdaptiveModel& model, BitWriter& bw) {
    std::ifstream file(in_model);
    if (!file) {
        std::cerr << "Couldn't open file for reading: " << in_model << ", aborting" << std::endl;
        throw std::ios_base::failure("Couldn't open file for reading");
    }
    bw.loadBitWriter(file);
    model.loadModel(file);
    file >> bytesRead >> bits_written >> a >> b >> s >> printProgress >> currentProgress;
}
void AdaptiveScalingCoder::storeModel(const std::filesystem::path& outModel, AdaptiveModel& model, BitWriter& bw) {
    std::ofstream file(outModel); //Note: text-based stream
    if(!file) {
        std::cerr << "Couldn't open file for writing: " << outModel << ", aborting" << std::endl;
        throw std::ios_base::failure("Couldn't open file for writing");
    }
    bw.storeBitWriter(file);
    model.writeModel(file);
    file << bytesRead << " " << bits_written << " " << a << " " << b << " " << s;
    file << " " << printProgress << " " <<  currentProgress;
}