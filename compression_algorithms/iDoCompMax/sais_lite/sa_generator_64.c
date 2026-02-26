/*
 * suftest.c for sais-lite
 * Copyright (c) 2008-2010 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "divsufsort/divsufsort.h"
#include "divsufsort64.h"

/* Checks the suffix array SA of the string T. */
static
int
sufcheck(const unsigned char *T, const unsigned int *SA, unsigned int n, int verbose) {
  unsigned int C[256];
  unsigned int i, p, q, t;
  unsigned int c;

  if(verbose) { fprintf(stderr, "sufcheck: "); }
  if(n == 0) {
    if(verbose) { fprintf(stderr, "Done.\n"); }
    return 0;
  }

  /* Check arguments. */
  if((T == NULL) || (SA == NULL) || (n < 0)) {
    if(verbose) { fprintf(stderr, "Invalid arguments.\n"); }
    return -1;
  }

  /* check range: [0..n-1] */
  for(i = 0; i < n; ++i) {
    if((SA[i] < 0) || (n <= SA[i])) {
      if(verbose) {
        fprintf(stderr, "Out of the range [0,%d].\n"
                        "  SA[%d]=%d\n",
                        n - 1, i, SA[i]);
      }
      return -2;
    }
  }

  /* check first characters. */
  for(i = 1; i < n; ++i) {
    if(T[SA[i - 1]] > T[SA[i]]) {
      if(verbose) {
        fprintf(stderr, "Suffixes in wrong order.\n"
                        "  T[SA[%d]=%d]=%d > T[SA[%d]=%d]=%d\n",
                        i - 1, SA[i - 1], T[SA[i - 1]], i, SA[i], T[SA[i]]);
      }
      return -3;
    }
  }

  /* check suffixes. */
  for(i = 0; i < 256; ++i) { C[i] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(i = 0, p = 0; i < 256; ++i) {
    t = C[i];
    C[i] = p;
    p += t;
  }

  q = C[T[n - 1]];
  C[T[n - 1]] += 1;
  for(i = 0; i < n; ++i) {
    p = SA[i];
    if(0 < p) {
      c = T[--p];
      t = C[c];
    } else {
      c = T[p = n - 1];
      t = q;
    }
    if((t < 0) || (p != SA[t])) {
      if(verbose) {
        fprintf(stderr, "Suffix in wrong position.\n"
                        "  SA[%d]=%d or\n"
                        "  SA[%d]=%d\n",
                        t, (0 <= t) ? SA[t] : -1, i, SA[i]);
      }
      return -4;
    }
    if(t != q) {
      ++C[c];
      if((n <= C[c]) || (T[SA[C[c]]] != c)) { C[c] = -1; }
    }
  }

  if(1 <= verbose) { fprintf(stderr, "Done.\n"); }
  return 0;
}

static
void
print_help(const char *progname, int status) {
  fprintf(stderr, "usage: %s FILE\n\n", progname);
  exit(status);
}

int
main(int argc, const char *argv[]) {
  FILE *fp, *fop;
  const char *fname;
  unsigned char *T;
  uint64_t *SA;
  uint64_t n;
  clock_t start, finish;

  /* Check arguments. */
  if((argc == 1) ||
     (strcmp(argv[1], "-h") == 0) ||
     (strcmp(argv[1], "--help") == 0)) { print_help(argv[0], EXIT_SUCCESS); }
  if(argc != 3) { print_help(argv[0], EXIT_FAILURE); }

  /* Open a file for reading. */
  if((fp = fopen(fname = argv[1], "rb")) == NULL) {
    fprintf(stderr, "%s: Cannot open file `%s': ", argv[0], fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }
    
    /* Open a file for reading. */
    if((fop = fopen(fname = argv[2], "wb")) == NULL) {
        fprintf(stderr, "%s: Cannot open file `%s': ", argv[0], fname);
        perror(NULL);
        exit(EXIT_FAILURE);
    }

  /* Get the file size. */
  if(fseek(fp, 0, SEEK_END) == 0) {
    n = ftell(fp);
    rewind(fp);
    if(n < 0) {
      fprintf(stderr, "%s: Cannot ftell `%s': ", argv[0], fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  } else {
    fprintf(stderr, "%s: Cannot fseek `%s': ", argv[0], fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  /* Max: Ignore the FASTA line if it is there, and advance the file pointer to the
   * beginning of the sequence */
  //Peek

  unsigned char c = fgetc(fp);
  ungetc(c, fp);
  if (c == '>') {
      char *fasta_line = NULL;
      size_t n_alloc = 0;
      ssize_t n_read = getline(&fasta_line, &n_alloc, fp);
      if (n_read == -1) {
          fprintf(stderr, "Error reading FASTA line\n");
          perror(NULL);
          exit(EXIT_FAILURE);
      }
      //Decrease n
      n -= n_read;
      free(fasta_line);
  }

  if (n <= INT32_MAX) {
      /* Allocate 5n bytes of memory. */
      T = (unsigned char *)malloc(n * sizeof(unsigned char));
      SA = (uint64_t *)malloc(n * sizeof(int));
  } else {
      T = (unsigned char *)malloc(n * sizeof(unsigned char));
      SA = (uint64_t*) malloc( n << 3);
  }
  if((T == NULL) || (SA == NULL)) {
      fprintf(stderr, "%s: Cannot allocate memory.\n", argv[0]);
      exit(EXIT_FAILURE);
  }

  /* Read n bytes of data. */
  if(fread(T, sizeof(unsigned char), (size_t)n, fp) != (size_t)n) {
    fprintf(stderr, "%s: %s `%s': ",
      argv[0],
      (ferror(fp) || !feof(fp)) ? "Cannot read from" : "Unexpected EOF in",
      argv[1]);
    perror(NULL);
    exit(EXIT_FAILURE);
  }
  fclose(fp);

  /* Construct the suffix array. */
  fprintf(stderr, "%s: %ld bytes ... ", fname, n);
  start = clock();
  if (n <= INT32_MAX) {
      if (divsufsort(T, (int*) SA, n) != 0) {
          fprintf(stderr, "%s: Error building SA.\n", argv[0]);
          exit(EXIT_FAILURE);
      }
  } else {
      if (divsufsort64(T, SA, n) != 0) {
          fprintf(stderr, "%s: Error building SA.\n", argv[0]);
          exit(EXIT_FAILURE);
      }
  }
  finish = clock();
  fprintf(stderr, "%.4f sec\n", (double)(finish - start) / (double)CLOCKS_PER_SEC);

    /* Check the suffix array. */
  if (n <= INT32_MAX) {
      if (sufcheck(T, (int*)SA, (int)n, 1) != 0) {
        fprintf(stderr, "Sufcheck failed!!!");
        exit(EXIT_FAILURE);
      }
  } else {
      if (sufcheck64(T, SA, n, 1) != 0) {
          fprintf(stderr, "Sufcheck failed!!!");
          exit(EXIT_FAILURE);
      }
  }

  if (n <= INT32_MAX) {
      /* Write into file */
      fwrite(SA, n, sizeof(int), fop);
  } else if (n <= UINT32_MAX) {
      /* Resize SA if it fits in 32 bits */
      uint32_t *int_ptr = (uint32_t*)SA;
      uint64_t *long_ptr = SA;
      for (uint64_t i = 0; i < n; i++) {
          *int_ptr++ = (unsigned int) *long_ptr++;
      }
      /* Write into file */
      fwrite(SA, n, sizeof(unsigned int), fop);
  } else {
      /* Write into file (longs) */
      fwrite(SA, n, 8, fop);
  }

  /* Deallocate memory. */
  free(SA);
  free(T);

  return 0;
}
