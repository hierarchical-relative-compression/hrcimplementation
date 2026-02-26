#include "BitWriter.hpp"
#include <iostream>
#include <stdio.h>
#include <filesystem>

BitWriter::BitWriter(std::string path)
	: m_bitBuffer(BIT_BUFFER_CAPACITY), m_stats(256)
{
	open(path);
}

void BitWriter::open(const std::string& path) {
    this->path = path;
    if (std::filesystem::exists(path))
        std::cerr << "Warning: file already exists: " << path << std::endl;

    m_fileStream.open(path, std::ios_base::trunc);
    if (!m_fileStream.is_open()) {
        perror(("Perror in C++ opening file: " + path).c_str());
        std::cerr << "Problem with path " << path << " at BitWriter" << std::endl;
        FILE *trial = fopen(path.c_str(), "w");
        if (trial == NULL) {
            perror(("Error in fopen for file " + path).c_str());
            throw std::runtime_error("Cannot open file: " + path);
        } else {
            std::cerr << "No error in C fopen!" << std::endl;
        }
        throw std::runtime_error("Cannot open file: " + path);
    }
}

BitWriter::BitWriter() : m_bitBuffer(BIT_BUFFER_CAPACITY), m_stats(256) {}

void BitWriter::write(bool bit)
{
	if (m_currentByte >= 0 && m_currentByte < m_stats.size())
		m_stats[m_currentByte].writeCounter++;

	m_bitBuffer << bit;
	if (m_bitBuffer.size() >= BIT_BUFFER_CAPACITY)
	{
		byte_t byte = m_bitBuffer.readByte();
		m_fileStream << byte;
	}
}

void BitWriter::write(int bit)
{
	if (bit == 0)
		write(false);
	else if (bit == 1)
		write(true);
	else
		throw std::invalid_argument("bit value can only be 1 or 0");
}

BitWriter& BitWriter::operator<<(bool bit)
{
	write(bit);
	return *this;
}

BitWriter& BitWriter::operator<<(int bit)
{
	write(bit);
	return *this;
}

void BitWriter::writeN(bool bit, int n)
{
	while (n--) {
		write(bit);
	}
}

void BitWriter::writeN(int bit, int n)
{
	writeN(bit == 0 ? false : true, n);
}

void BitWriter::flush()
{
	auto bytes = m_bitBuffer.readAllBytes(0);
	for (auto byte : bytes)
		m_fileStream << byte;

	m_fileStream.flush();
}

void BitWriter::beginByte(int byte)
{
	if (byte >= 0 && byte < m_stats.size())
	{
		m_currentByte = byte;
		m_stats[m_currentByte].readCounter += 8;
	}
}

std::vector<BitStat> BitWriter::getStats()
{
	return m_stats;
}

BitWriter::~BitWriter()
{
	flush();
}


void BitWriter::storeBitWriter(std::ostream& file) {
    file << path << "\n";
    file << m_currentByte << " ";
    size_t statsSize = m_stats.size();
    file << statsSize << " ";
    for (size_t i = 0; i < statsSize; i++) {
        file << m_stats[i].readCounter << " " << m_stats[i].writeCounter << " ";
    }
    m_bitBuffer.storeBitBuffer(file);
}
void BitWriter::loadBitWriter(std::istream& file) {
    std::getline(file, path);
    file >> m_currentByte;
    size_t statsSize;
    file >> statsSize;
    m_stats = std::vector<BitStat>(statsSize);
    for (size_t i = 0; i < statsSize; i++) {
        file >> m_stats[i].readCounter >> m_stats[i].writeCounter;
    }
    m_bitBuffer.loadBitBuffer(file);
}

