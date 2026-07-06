
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <chrono>
#include <ctime>

#include <rlz/utils.h>
#include <rlz/mmsearch.h>

static data_type *_x;
static unsigned int *_SA;
static unsigned int _n;
static data_type *_sx;
static filelength_type _sn;
static unsigned int _ceil_log_n_plus_1;

static unsigned int *_LCP; // LCP[i] = lcp(SA[i], SA[i + 1])
static unsigned int *_LBSTLCP;
static unsigned int *_RBSTLCP;
static unsigned int *_INVSA;

void computeLZFactorAtLCP(filelength_type i, filelength_type &pos, filelength_type &len);

int renormalizations = 0;
double _average_choices = 0;
int _last_pos;
int _non_ascending_runs;

uint32_t maxFactorLength = 0;
int longFactorLength = 0;
int longFactors = 0;
int lenZeroFactors = 0;

void constructInvSa() {
	_INVSA = new unsigned int[_n];
	for(unsigned int i = 0; i < _n; ++i) {
		_INVSA[_SA[i]] = i; 
	}
}

// Kasais algorithm
void constructLCP() {
	_LCP = new unsigned int[_n];
	unsigned int k = 0;
	for(unsigned int i = 0; i < _n; ++i) {
		if(_INVSA[i] == _n) {
			k = 0;
			continue;
		}
    int j = _SA[_INVSA[i]+1];

    while (i+k<_n && j+k<_n && _x[i+k]==_x[j+k])
      k++;
 
    _LCP[_INVSA[i]] = k;
 
    if (k>0)
      k--;
	}
}

void BSTRec(unsigned int i, unsigned int left, unsigned int right) {
	auto mid = (left + right) >> 1;
  if (left == right) {
    _LBSTLCP[i] = _n - _SA[left];
    _RBSTLCP[i] = _n - _SA[left];
  }
  else if (left + 1 == right) {
    _LBSTLCP[i] = _n - _SA[left];
		_RBSTLCP[i] =  _LCP[mid];
	}
	else {
    BSTRec(i*2, left, mid);
    BSTRec(i*2+1, mid, right);
		_LBSTLCP[i] = std::min(_LBSTLCP[i*2], _RBSTLCP[i*2]);
		_RBSTLCP[i] = std::min(_LBSTLCP[i*2 + 1], _RBSTLCP[i*2 + 1]);
	}
}

void constructBST() {
	_LBSTLCP = new unsigned int[_n * 4];
	_RBSTLCP = new unsigned int[_n * 4];
	BSTRec(1, 0, _n - 1);
}

void lzInitialize(data_type *ax, unsigned int *sa, unsigned int an) {
    _x = ax;
    _n = an;
    _SA = sa;

    _ceil_log_n_plus_1 = ceil(log(_n + 1) / log(2));

    constructInvSa();
    constructLCP();
    constructBST();

    //TODO: preprocess SA and store intervals containing all suffixes with a given short prefix
}

int lzFactorize(char *fileToParse, int seqno) {
    FILE *infile = fopen(fileToParse, "r");
    if (!infile) {
        fprintf(stderr, "Error opening file of sequence %d (%s)\n", seqno, fileToParse);
        exit(1);
    }
    filelength_type sn = 0;
    fseek(infile, 0L, SEEK_END);
    sn = ftello(infile) / sizeof(data_type);
    std::cerr << "sn: " << sn << '\n';
    fseek(infile, 0L, SEEK_SET);
    data_type *sx = new data_type[sn + 1];
    if (sn != fread(sx, sizeof(data_type), sn, infile)) {
        fprintf(stderr, "Error reading %lu bytes from file %s\n", static_cast<long unsigned int>(sn), fileToParse);
        exit(1);
    }
    sx[sn] = 1; //??? 22.03.2021
    fclose(infile);

    std::cerr << "File is in memory\n";
    auto t1 = std::chrono::high_resolution_clock::now();

    _sx = sx;
    _sn = sn;

    _average_choices = 0;

    //to record how much of the reference was actually used
    int *covered = new int[_n];
    for(unsigned int c = 0; c < _n; c++) {
        covered[c] = 0;
    }

    std::vector<uint32_t> sources;
    std::vector<filelength_type> starts;
    filelength_type i = 0;
    unsigned int numfactors = 0;

    unsigned int inc = 100000000;
    uint64_t mark = inc;

    std::cerr << "About to start main parsing loop...\n";

    while(i < _sn) {
        if(i > mark){
           fprintf(stderr,"i = %lu; numFactors = %u\n",static_cast<long unsigned int>(i),numfactors);
           mark = mark + inc;
        }
        filelength_type pos = 0, len = 0;
        computeLZFactorAtLCP(i, pos, len);
        //std::cerr << "Factor computed: " << i << " pos: " << pos << " len: " << len << '\n';
        //std::cerr << pos << ',' << len << ',';
        numfactors++;
        //if (len > 0) {
        //    for (unsigned int c = pos; (c < (pos + len)); c++) {
        //        covered[c]++;
        //    }
        //    longFactors++;
        //    longFactorLength += len;
        //}
        //starts.push_back(i);
        //if(len == 0){ pos = (((uint32_t)_sx[i]) | (1<<31));}
        //sources.push_back(pos);
        if (len == 0){
           lenZeroFactors++;
           len++;
        }
        i = i + len;
        if (maxFactorLength < len)
            maxFactorLength = len;
    }
    std::cerr << '\n';

    auto t2 = std::chrono::high_resolution_clock::now();
    uint64_t lzFactorizeTime = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();


    delete[] sx;

    unsigned int nsegments = 0;
    unsigned int saved = 0;
    unsigned int phrasesSaved = 0;
    unsigned int si = 0;
    for (unsigned int j = 0; j < _n; j++) {
        unsigned int start = j;
        while (!covered[start] && start < _n) {
            start++;
        }
        unsigned int end = start;
        while (covered[end] && end < _n) {
            end++;
        }
        //cerr << "segment from: " << start << " to: " << end << '\n';
        j = end;
        while (starts[si] < start && si < starts.size()) si++;
        while (starts[si] < end && si < starts.size()) {
            int length = (starts[si + 1] - starts[si]);
            if (starts[si] + length < end) {
                saved += length;
                phrasesSaved++;
                //cerr << "\t\tsaved: (" << starts[si] << "," << starts[si+1]-1 << ")" << '\n';
            }
            si++;
        }
        nsegments++;
    }
    //std::cerr << "nsegments = " << nsegments << '\n';
    //cerr << "saved = " << saved << '\n';
    //std::cerr << "phrases saved = " << phrasesSaved << '\n';


    ////for a reference without redundant symbols
    //_unusedSymbols = 0;
    //_numberOfUnused = new uint32_t[_n];
    //uint32_t positionsCovered = 0;
    //uint32_t positionsCoveredTwice = 0;
    //uint32_t positionsCoveredTenTimesOrMore = 0;
    //for (unsigned int c = 0; c < _n; c++) {
    //    if (covered[c]) {
    //        positionsCovered++;
    //        _x[c - _unusedSymbols] = _x[c]; //remove unused symbols from the reference
    //    } else {
    //        _unusedSymbols++;
    //    }
    //    _numberOfUnused[c] = _unusedSymbols;
    //    if (covered[c] > 1) positionsCoveredTwice++;
    //    if (covered[c] > 10) positionsCoveredTenTimesOrMore++;
    //}

    //for (size_t i = 0; i < sources.size(); i++) {
    //    if (sources[i] != starts[i]) {
    //        sources[i] -= _numberOfUnused[sources[i]];
    //    }
    //}

    //Serialize reference, phrases, and other info
    //File format:
    //size_t _r: length of reference
    //size_t [] _reference: _r uint8_ts, the referene
    //size_t _r: length of reference <--- yes, again
    //size_t [] sa: _r uint32_ts, the suffix array of the reference (discarded after load)
    //size_t _z: number of phrases
    //size_t [] starts: starting positions of phrases in collection (discarded after load)
    //size_t _z: number of phrases <--- yes, again
    //uint32_t [] _phrases: sources of phrases (positions in reference)
    //size_t _n: total length of the collection including separator symbols

    //std::cerr << "Writing out to file: " << outputfilename << std::endl;
    //std::ofstream fout(outputfilename, std::ios::out | std::ios::binary);

    //std::cout << "Reference: " << _n << " items, file size =  " <<  _n * sizeof(unsigned int) << "\n";
    ////size_t size = positionsCovered; // for the reference without unused symbols
    //size_t size = _n;
    //std::cout << "Writing out reference: ";
    //for (int j = 0; j < 10; j++) std::cout << _x[j] << ' ';
    //std::cout << " ... (" << size << " items)\n";
    //fout.write((char *) &size, sizeof(size_t));
    //fout.write((char *) &_x[0], size * sizeof(data_type));

    //std::cout << "Writing out _SA: ";
    //for (int j = 0; j < 10; j++) std::cout << _SA[j] << ' ';
    //std::cout << " ... (" << size << " items)\n";
    //fout.write((char *) &size, sizeof(size_t));
    //fout.write((char *) _SA, size * sizeof(uint32_t));

    //size = starts.size();
    //std::cout << "Writing out starts: ";
    //for (int j = 0; j < 10; j++) std::cout << starts[j] << ' ';
    //std::cout << " ... (" << size << " items)\n";
    //fout.write((char *) &size, sizeof(size_t));
    //fout.write((char *) &starts[0], starts.size() * sizeof(filelength_type));

    //size = sources.size();
    //std::cout << "Writing out sources: ";
    //for (int j = 0; j < 10; j++) std::cout << sources[j] << ' ';
    //std::cout << " ... (" << size << " items)\n";
    //fout.write((char *) &size, sizeof(size_t));
    //fout.write((char *) &sources[0], sources.size() * sizeof(uint32_t));


    //fout.write((char *) &_sn, sizeof(size_t));

    //fout.close();

    
    fprintf(stderr, "number of factors = %d\n", numfactors);
    fprintf(stderr, "average factor length = %ld\n", static_cast<long unsigned int>(_sn)/numfactors);
    fprintf(stderr, "max. factor length = %d\n", maxFactorLength);
    //fprintf(stderr, "longFactors = %d\n", longFactors);
    //fprintf(stderr, "longFactorLength = %d\n", longFactorLength);
    //fprintf(stderr, "lenZeroFactors = %d\n", lenZeroFactors);

    //fprintf(stderr, "positionsCovered = %d; positions covered twice = %d; positions > 10 times = %d;\n",
    //        positionsCovered, positionsCoveredTwice, positionsCoveredTenTimesOrMore);
    
    fprintf(stderr, "---Time to lz factorize = %lu milliseconds\n", lzFactorizeTime);
    


    //fprintf(stdout, "%u,%d,%d,%d,%d,%.2f,%u,%u,%.2f,%d,%d,%d,%lu,%lu,",
    //        numfactors, positionsCovered, positionsCovered, positionsCoveredTwice, positionsCoveredTenTimesOrMore,
    //        ((double)positionsCovered / _n) * 100, _unusedSymbols, phrasesSaved,
    //        ((double)(_sn/numfactors)), longFactors, longFactorLength, lenZeroFactors,
    //        positionsCovered * sizeof(unsigned int) + sizeof(size_t), starts.size() * 2 * sizeof(unsigned int) + 2 * sizeof(size_t)
    //);
    //fprintf(stdout, "\n");
    delete[] covered;
    return numfactors;
}

bool extendMatch(data_type *suffix, filelength_type seq_pos, filelength_type &len) {
	auto seq_start = _sx + seq_pos;
	// Optimisation no need to check if past array when last elements of each array is unmatchable
	while(seq_start[len] == suffix[len]) {
		++len;
	}
	return seq_start[len] < suffix[len];
}

void computeLZFactorAtLCP(filelength_type i, filelength_type &pos, filelength_type &len) {
  unsigned int left = 0, right = _n - 1, ilcp = 1;
  filelength_type matchl = 0, matchr = 0;
  extendMatch(_x + _SA[0], i, matchl);
  while(left + 1 < right) {
    auto mid = (left + right) >> 1;
    if(matchl >= matchr) {
	    if(matchl < _LBSTLCP[ilcp]) {
        left = mid;
        ilcp += ilcp + 1;
	    }
      else if(matchl > _LBSTLCP[ilcp]) {
        right = mid;
        ilcp += ilcp;
	    }
      else {
        auto tmp = matchl;
        if(extendMatch(_x + _SA[mid], i, tmp)) {
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
	    if(matchr < _RBSTLCP[ilcp]) {
        right = mid;
        ilcp += ilcp;
	    }
      else if(matchr > _RBSTLCP[ilcp]) {
        left = mid;
        ilcp += ilcp + 1;
	    }
      else {
        auto tmp = matchr;
        if(extendMatch(_x + _SA[mid], i, tmp)) {
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
  extendMatch(_x + _SA[left], i, matchl);
  extendMatch(_x + _SA[right], i, matchr);
  if(matchl >= matchr) {
    pos = _SA[left];
    len = matchl;
  }
  else {
    pos = _SA[right];
    len = matchr;
  }
}
