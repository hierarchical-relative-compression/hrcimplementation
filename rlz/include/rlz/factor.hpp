#pragma once
#include"common.h"
#include<ostream>
#include<iostream>

class FactorWriter {
public:
  virtual void write(data_type c) = 0;
  virtual void write(filelength_type pos, filelength_type len) = 0;
  virtual void write_string(const std::string& s) = 0;
};

class FactorSink: public FactorWriter {
  filelength_type factors = 0;
  filelength_type single_count = 0;
public:
  void write(data_type) {
    ++single_count;
    ++factors;
  }
  void write(filelength_type, filelength_type) {
    ++factors;
  }
  void write_string(const std::string& s) {
      factors += s.size();
  }
  filelength_type factor_count() {
    return factors;
  }
};

class FactorStreamWriter: public FactorSink {
  std::ostream* stream;
public:
  FactorStreamWriter(std::ostream* stream): stream(stream) {}
  void write(data_type c) override {
    FactorSink::write(c);
    filelength_type enc_c = c;
    stream->write((char*) &enc_c, sizeof(enc_c));
    enc_c = 0;
    stream->write((char*) &enc_c, sizeof(enc_c));
  }
  void write(filelength_type pos, filelength_type len) {
    FactorSink::write(pos, len);
    stream->write((char*) &pos, sizeof(pos));
    stream->write((char*) &len, sizeof(len));
  }
  void write_string(const std::string& s) override {
      stream->write(s.c_str(), s.size());
  }
};
