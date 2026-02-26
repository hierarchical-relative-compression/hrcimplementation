
#ifndef MM_SEARCH_H
#define MM_SEARCH_H

#include <rlz/rlz.hpp>

void lzInitialize(data_type *ax, unsigned int *sa, unsigned int an);

//returns the length of the RLZ parsing of sx relative to ax
int lzFactorize(char *fileToParse, int seqno);

#endif
