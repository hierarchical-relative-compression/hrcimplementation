#include<rlz/rlz.hpp>
#include<algorithm>
#include<iostream>
#include<fstream>
#include<string>

DynArray<unsigned int> constructLCP(const DynArray<data_type>& ref_seq, const DynArray<unsigned int>& sa);

void RLZ::parseDNA(std::istream& file) {
    //Get size of the file in bytes
    file.seekg(0, file.end);
    std::size_t length = file.tellg();
    //Go to the start and read the first FASTA definition line if it is there
    file.seekg(0, file.beg);
    char c = file.peek();
    if (c == '>') {
        //There's a FASTA definition line that we are going to ignore
        std::string line;
        std::getline(file, line); //Extracts the end of line
        //The remaining length is smaller
        std::size_t curr_pos = file.tellg();
        length = length - curr_pos;
    }
    //Allocate and read the rest in one go
    DynArray<data_type> buf(length + 1);
    file.read((char *) buf.begin(), length);
    buf[length] = 0;
    _ref_seq = std::move(buf);
    last_ch = _ref_seq.begin() + length;
}

RLZ::RLZ(std::istream& in) {
    parseDNA(in);
}

RLZ::RLZ(const std::filesystem::path& ref) {
    //Same as read_compacted_DNA
    std::ifstream file;
    //Only crash if there's an exception
    file.exceptions(std::ifstream::badbit);
    if (!std::filesystem::exists(ref)) {
        std::cerr << "ERROR: File not found: " << ref.string() << std::endl;
        std::exit(1);
    }
    //Open in binary because we are going to do unformatted input
    file.open(ref, std::ios_base::binary | std::ios_base::in);
    if (!file) {
        std::cerr << "IO Error reading file: " << ref << std::endl;
        std::exit(1);
    }
    parseDNA(file);
}

RLZCompress::RLZCompress(const std::filesystem::path& path, bool decomp) : RLZ(path), RefDataStructure(path) {
    if (!decomp) {
        //Need to load SA
        std::filesystem::path ref_sa_path = path.parent_path() / (path.filename().string() + ".sa");
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
        file.open(ref_sa_path);
        file.seekg(0, file.end);
        filelength_type file_len = file.tellg();
        filelength_type sa_len =  file_len / sizeof(unsigned int);
        file.seekg(0, file.beg);
        DynArray<unsigned int> ref_sa(sa_len);
        file.read((char*) ref_sa.begin(), file_len);
        file.close();
        _ref_sa = std::move(ref_sa);
        _bst_lcp = DynArray<std::pair<unsigned int, unsigned int>>(_ref_sa.size() * 4);
        DynArray<unsigned int> lcp = constructLCP(_ref_seq, _ref_sa);
        constructBST(lcp, 1, 0, _ref_sa.size() - 1);
    }
}
RLZCompress::RLZCompress(std::istream& in, bool decomp) : RLZ(in), RefDataStructure(in) {
    if (!decomp)
        throw std::runtime_error("RLZ: Can't compress without SA!!!");
}
RLZCompress::~RLZCompress() {}

void RLZCompress::parseRef(std::istream& in) {}
void RLZCompress::compress_target(const std::filesystem::path& in, std::ostream& out) const {
    FactorStreamWriter fsw(&out);
    compress_fasta(in.c_str(), fsw);
}

void RLZCompress::decompress_target(std::istream& comp_tar, std::ostream& decomp_tar) const {
    if (comp_tar.peek() == '>') {
        std::string line;
        std::getline(comp_tar, line);
        decomp_tar.write(line.c_str(), line.size());
        decomp_tar.write("\n", 1);
    }
    filelength_type pos, len;
    comp_tar.read((char *)&pos, sizeof(pos));
    while (comp_tar) {
        comp_tar.read((char *)&len, sizeof(len));
        if (len == 0) {
            decomp_tar.put(pos);
        } else {
            decomp_tar.write(reinterpret_cast<const char *>(_ref_seq.begin() + pos), len);
        }
        //Prepare for next iteration
        comp_tar.read((char *)&pos, sizeof(pos));
    }
}

void RLZCompress::set_id(const std::string& new_id) {
    id = new_id;
}

const std::string& RLZCompress::get_id() {
    return id;
}

bool extendMatch(const data_type *text1, const data_type* text2, filelength_type &len) {
	// Optimisation no need to check if past array when last elements of each array is unmatchable
    // why would they be unmatchable?
	while(text1[len] == text2[len]) {
		++len;
	}
	return text1[len] < text2[len];
}

DynArray<unsigned int> constructInvSa(const DynArray<unsigned int>& sa) {
	DynArray<unsigned int> invsa(sa.size());
	for(unsigned int i = 0; i < sa.size(); ++i) {
		invsa[sa[i]] = i; 
	}
  return invsa;
}

// kasais algorithm
DynArray<unsigned int> constructLCP(const DynArray<data_type>& ref_seq, const DynArray<unsigned int>& sa) {
  auto invsa = constructInvSa(sa);
	DynArray<unsigned int> lcp(sa.size());
	unsigned int k = 0;
	for(unsigned int i = 0; i < sa.size(); ++i) {
		if(invsa[i] + 1 == sa.size()) {
			k = 0;
			continue;
		}
    auto j = sa[invsa[i]+1];

    while (i+k<sa.size() && j+k<sa.size() && ref_seq[i+k]==ref_seq[j+k])
      k++;
 
    lcp[invsa[i]] = k;
 
    if (k>0)
      k--;
	}
  return lcp;
}

void RLZCompress::constructBST(const DynArray<unsigned int>& lcp, filelength_type i, filelength_type left, filelength_type right) {
	auto mid = (left + right) >> 1;
  if (left == right) {
    _bst_lcp[i].first = _ref_seq.size() - _ref_sa[left];
    _bst_lcp[i].second = _ref_seq.size() - _ref_sa[left];
  }
  else if (left + 1 == right) {
    _bst_lcp[i].first = _ref_seq.size() - _ref_sa[left];
		_bst_lcp[i].second =  lcp[mid];
	}
	else {
    constructBST(lcp, i*2, left, mid);
    constructBST(lcp, i*2+1, mid, right);
		_bst_lcp[i].first = std::min(_bst_lcp[i*2].first, _bst_lcp[i*2].second);
		_bst_lcp[i].second = std::min(_bst_lcp[i*2 + 1].first, _bst_lcp[i*2 + 1].second);
	}
}

//A copy of the code below, but allocating the object in the heap
std::unique_ptr<RLZCompress> RLZCompress::from_dynamic(const std::filesystem::path& ref_seq_path, const std::filesystem::path& ref_sa_path) {
  filelength_type ref_len = 0;
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

  file.open(ref_seq_path);
  file.seekg(0, file.end);
  ref_len = file.tellg() / sizeof(data_type);
  file.seekg(0, file.beg);

  DynArray<data_type> ref_seq(ref_len + 1);
  file.read((char*) ref_seq.begin(), ref_len);
  ref_seq[ref_len] = 0;
  file.close();

  file.open(ref_sa_path);
  file.seekg(0, file.end);
  filelength_type file_len = file.tellg();
  filelength_type sa_len =  file_len / sizeof(unsigned int);
  file.seekg(0, file.beg);
  DynArray<unsigned int> ref_sa(sa_len);
  file.read((char*) ref_sa.begin(), file_len);
  file.close();
  auto pointer = std::make_unique<RLZCompress>(std::move(ref_seq), std::move(ref_sa));
  return pointer;
}

//A copy of the code below, but treating the first FASTA definition line
std::unique_ptr<RLZCompress> RLZCompress::from_dynamic_fasta(const std::filesystem::path& ref_seq_path, const std::filesystem::path& ref_sa_path) {
    filelength_type ref_len = 0;
    filelength_type fasta_len = 0;
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

    file.open(ref_seq_path);
    file.seekg(0, file.end);
    fasta_len = file.tellg() / sizeof(data_type);
    file.seekg(0, file.beg);
    if (fasta_len == 0) {
        throw std::runtime_error("File " + ref_seq_path.string() + " is empty!!!");
    }
    char c = file.peek();
    if (c == '>') {
        //skip the first line
        std::string fasta_line;
        std::getline(file, fasta_line);
        filelength_type curr_pos = file.tellg();
        ref_len = fasta_len - curr_pos;
    } else {
        ref_len = fasta_len;
    }
    DynArray<data_type> ref_seq(ref_len + 1);
    file.read((char*) ref_seq.begin(), ref_len);
    ref_seq[ref_len] = 0;
    file.close();

    file.open(ref_sa_path);
    file.seekg(0, file.end);
    filelength_type file_len = file.tellg();
    filelength_type sa_len =  file_len / sizeof(unsigned int);
    file.seekg(0, file.beg);
    DynArray<unsigned int> ref_sa(sa_len);
    file.read((char*) ref_sa.begin(), file_len);
    file.close();
    auto pointer = std::make_unique<RLZCompress>(std::move(ref_seq), std::move(ref_sa));
    return pointer;
}

RLZCompress RLZCompress::from(const std::filesystem::path& ref_seq_path,  const std::filesystem::path& ref_sa_path) {
  filelength_type ref_len = 0;
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

  file.open(ref_seq_path);
  file.seekg(0, file.end);
  ref_len = file.tellg() / sizeof(data_type);
  file.seekg(0, file.beg);

  DynArray<data_type> ref_seq(ref_len + 1);
  file.read((char*) ref_seq.begin(), ref_len);
  ref_seq[ref_len] = 0;
  file.close();

  file.open(ref_sa_path);
  file.seekg(0, file.end);
  filelength_type file_len = file.tellg();
  filelength_type sa_len =  file_len / sizeof(unsigned int);
  file.seekg(0, file.beg);
  DynArray<unsigned int> ref_sa(sa_len);
  file.read((char*) ref_sa.begin(), file_len);
  file.close();

  return RLZCompress(std::move(ref_seq), std::move(ref_sa));
}

RLZCompress::RLZCompress(DynArray<data_type>&& ref_seq, DynArray<unsigned int>&& ref_sa): RLZ(std::move(ref_seq)), RefDataStructure("/"), _ref_sa(std::move(ref_sa)), _bst_lcp(_ref_sa.size() * 4) {
  auto lcp = constructLCP(_ref_seq, _ref_sa);
	constructBST(lcp, 1, 0, _ref_sa.size() - 1);
}

filelength_type RLZCompress::ref_len() const {
  return _ref_seq.size() - 1; // padded with ending character
}

std::size_t RLZCompress::compress(const char *file_to_parse, FactorWriter& writer) {
  FILE *infile = fopen(file_to_parse, "r");
  if (!infile) {
      fprintf(stderr, "Error opening file %s\n", file_to_parse);
      exit(1);
  }
  filelength_type seq_len = 0;
  fseek(infile, 0L, SEEK_END);

  seq_len = ftello(infile) / sizeof(data_type);

  fseek(infile, 0L, SEEK_SET);
  data_type *seq = new data_type[seq_len + 1];
  if (seq_len != fread(seq, sizeof(data_type), seq_len, infile)) {
      fprintf(stderr, "Error reading %lu bytes from file %s\n", seq_len, file_to_parse);
      exit(1);
  }
  //Explanation of this: If we put an additional char on the reference set to 0, and an additional char on the target set
  //to 1, the last char does not match. Then, we do not have to check for lengths on "extend match"
  seq[seq_len] = 1; //??? 22.03.2021
  fclose(infile);

    //Debug: check for out of bounds
    last_ch = _ref_seq.begin() + _ref_seq.size() - 1;

  filelength_type i = 0;
  while(i < seq_len) {
    filelength_type pos = 0, len = 0;
    computeLZFactorAt(seq, i, pos, len);
    if (len == 0){
      writer.write(seq[i]);
      ++i;
    }
    else {
      writer.write(pos, len);
      i += len;
    }
  }

  delete[] seq;
  return seq_len;
}

std::size_t RLZCompress::compress_fasta(const char *file_to_parse, FactorWriter& writer) const {
    FILE *infile = fopen(file_to_parse, "r");
    if (!infile) {
        fprintf(stderr, "Error opening file %s\n", file_to_parse);
        exit(1);
    }
    filelength_type fasta_len = 0;
    filelength_type seq_len = 0;
    fseek(infile, 0L, SEEK_END);

    fasta_len = ftello(infile) / sizeof(data_type);

    fseek(infile, 0L, SEEK_SET);
    if (fasta_len == 0) {
        throw std::runtime_error("File " + std::string(file_to_parse) + " is empty!!!");
    }
    //Peek first character, to check FASTA definition line
    char c;
    c = fgetc(infile);
    ungetc(c, infile);
    if (c == '>') {
        //Write FASTA line, if it's there
        char *fasta_line = NULL;
        size_t alloc;
        ssize_t len;
        if ((len = getline(&fasta_line, &alloc, infile)) != -1) { //Includes the newline character
            seq_len = fasta_len - len;
            //Write line, and make sure ints are aligned
            std::string line(fasta_line, len);
            int rem = 4 - (line.size() % 4);
            if (rem != 4) {
                line.insert(line.size() - 1, rem, ' '); //Insert 'rem' copies of space character before the newline
            }
            writer.write_string(line);
            free(fasta_line);
        } else {
            free(fasta_line);
            throw std::runtime_error("Failed to read " + std::string(file_to_parse));
        }
    } else {
        seq_len = fasta_len;
    }
    data_type *seq = new data_type[seq_len + 1];
    if (seq_len != fread(seq, sizeof(data_type), seq_len, infile)) {
        fprintf(stderr, "Error reading %lu bytes from file %s\n", seq_len, file_to_parse);
        exit(1);
    }
    seq[seq_len] = 1; //??? 22.03.2021
    fclose(infile);

    filelength_type i = 0;
    while(i < seq_len) {
        filelength_type pos = 0, len = 0;
        computeLZFactorAt(seq, i, pos, len);
        if (len == 0){
            writer.write(seq[i]);
            ++i;
        }
        else {
            writer.write(pos, len);
            i += len;
        }
    }

    delete[] seq;
    return seq_len;
}

void RLZCompress::computeLZFactorAt(data_type* seq, filelength_type i, filelength_type &pos, filelength_type &len) const
{
  unsigned int left = 0, right = _ref_sa.size() - 1, ilcp = 1;
  filelength_type matchl = 0, matchr = 0;
  extendMatch(seq + i, _ref_seq.begin() + _ref_sa[left], matchl);
  extendMatch(seq + i, _ref_seq.begin() + _ref_sa[right], matchr);
  while(left + 1 < right) {
    auto mid = (left + right) >> 1;
    if(matchl >= matchr) {
	    if(matchl < _bst_lcp[ilcp].first) {
        left = mid;
        ilcp += ilcp + 1;
	    }
      else if(matchl > _bst_lcp[ilcp].first) {
        right = mid;
        ilcp += ilcp;
	    }
      else {
        auto tmp = matchl;
        if(extendMatch(seq + i, _ref_seq.begin() + _ref_sa[mid], tmp)) {
          matchr = tmp;
          right = mid;
          ilcp += ilcp;
        }
        else {
          matchl = tmp;
          left = mid;
          ilcp += ilcp + 1;
        }
      }
	  }
    else {
	    if(matchr < _bst_lcp[ilcp].second) {
        right = mid;
        ilcp += ilcp;
	    }
      else if(matchr > _bst_lcp[ilcp].second) {
        left = mid;
        ilcp += ilcp + 1;
	    }
      else {
        auto tmp = matchr;
        if(extendMatch(seq + i, _ref_seq.begin() + _ref_sa[mid], tmp)) {
          matchr = tmp;
          right = mid;
          ilcp += ilcp;
        }
        else {
          matchl = tmp;
          left = mid;
          ilcp += ilcp + 1;
        }
      }
    }
  }
  if(left <_ref_sa.size())
    extendMatch(seq + i, _ref_seq.begin() + _ref_sa[left], matchl);
  if(right <_ref_sa.size())
    extendMatch(seq + i, _ref_seq.begin() + _ref_sa[right], matchr);
  if(matchl >= matchr) {
    pos = _ref_sa[left];
    len = matchl;
  }
  else {
    pos = _ref_sa[right];
    len = matchr;
  }
}

void RLZCompress::decompress(const std::filesystem::path &target_unizpped) {
    //First, read FASTA line

}