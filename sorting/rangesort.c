#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "sort.h"
#include <sys/types.h>
#include <sys/stat.h>

#define KEY_REC_SIZE 12
//#define CACHE_SIZE (6000 * 1000)
#define MAX_NUM_ENTRY 15

typedef struct __key_rec {
  unsigned int key;
  rec_t *record;
} key_rec;

void usage(char *prog) {
  fprintf(stderr, "Usage: %s -i inputfile -o outputfile -l low value"
	  "-h high value\n", prog);
  exit(1);
}

void err_fileopen(char *file) {
  fprintf(stderr, "Error: Cannot open file %s\n", file);
  exit(1);
}

void err_fileread(char *file) {
  fprintf(stderr, "Error: Read file %s failure\n", file);
  exit(1);
}

void err_filewrite(char *file) {
  fprintf(stderr, "Error: Write file %s failure\n", file);
  exit(1);
}

void err_range() {
  fprintf(stderr, "Error: Invalid range value\n");
  exit(1);
}

int key_cmpfunc (const void *a, const void *b) {
  return (((key_rec*)a)->key - ((key_rec*)b)->key);
}

int cmpfunc (const void *a, const void *b) {
  return (((rec_t*)a)->key - ((rec_t*)b)->key);
}

inline void key_swap (key_rec *a, key_rec *b) {
  key_rec temp = *a;
  *a = *b;
  *b = temp;
}

void key_insertsort(key_rec *base, size_t num) {
  int i = 0;
  for ( ; i < num; ++i) {
    int j = i;
    key_rec cur_key = base[i];
    while (j > 0 && base[j-1].key > cur_key.key) {
      base[j] = base[j-1];
      --j;
    }
    base[j] = cur_key;
  }  
}

void key_opt_qsort(key_rec *base, const size_t num) {
  if (num < MAX_NUM_ENTRY) {
    key_insertsort(base, num);
    return;
  }
  //if (num == 0) return;
  int pivot = num - 1;
  int i = -1;
  int j = num;
  while (i <= j) {
    while (base[++i].key < base[pivot].key);
    while (base[--j].key > base[pivot].key)
      if (j == 0) break;
    if (i >= j) break;
    key_swap(&base[i], &base[j]);
  }
  key_swap(&base[i], &base[pivot]);
  key_opt_qsort(base, i);
  key_opt_qsort(&base[i + 1], num - i - 1);
}


// Rec representation: 4-byte key + 96-byte record
int main(int argc, char *argv[]) {
  assert(sizeof(rec_t) == 100);
  if (argc != 9) usage("rangesort");
  // arguments
  char *inputfile = "/no/such/file";
  char *outputfile = "/no/such/file";
  long input_low = 0;
  long input_high = 0;
  unsigned int low_value = 0;
  unsigned int high_value = 0;

  // input params
  // TODO(winstonyan): Add input validation. File name and unsigned 32 int
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "i:o:l:h:")) != -1) {
    switch (c) {
    case 'i':
      inputfile  = strdup(optarg);
      break;
    case 'o':
      outputfile = strdup(optarg);
      break;
    case 'l':
      input_low = strtol(optarg, NULL, 10);
      if (input_low == 0 && strcmp(optarg, "0") != 0) err_range();
      if (input_low > UINT_MAX || input_low < 0) err_range();
      low_value = input_low;
      break;
    case 'h':
      input_high = strtol(optarg, NULL, 10);
      if (input_high == 0 && strcmp(optarg, "0") != 0) err_range();
      if (input_high > UINT_MAX || input_high < 0 || input_high < input_low)
	err_range();
      high_value = input_high;
      break;
    default:
      usage(argv[0]);
    }
  }

  // read input file
  clock_t t = clock();
  int inputfd = open(inputfile, O_RDONLY);
  if (inputfd < 0) err_fileopen(inputfile);
  // sort
  rec_t r;
  rec_t* records = (rec_t*)malloc(sizeof(rec_t));
  //key_rec* keys = (key_rec*)malloc(sizeof(key_rec));
  int num_of_rec = 0;
  while (1) {
    int rc;
    rc = read(inputfd, &r, sizeof(rec_t));
    if (rc == 0) break;
    if (rc < 0) err_fileread(inputfile);
    if (r.key >= low_value && r.key <= high_value) {
      ++num_of_rec;
      records = (rec_t*)realloc(records, num_of_rec * sizeof(rec_t));
      records[num_of_rec - 1] = r;
    }
    // TESTOUTPUT
    //printf("read in %d\n", num_of_rec);
  }
  
  // TESTOUTPUT
  //printf("%d records.\n", num_of_rec);

  close(inputfd);

  key_rec* keys = (key_rec*)malloc(num_of_rec * sizeof(key_rec));
  int i;
  for (i = 0; i < num_of_rec; ++i) {
    keys[i].key = records[i].key;
    keys[i].record = &records[i];
  }
  // base quick sort
  //qsort(keys, num_of_rec, sizeof(key_rec), key_cmpfunc);
  // optimized quick sort
  key_opt_qsort(keys, num_of_rec);
  
  //TESTOUTPUT
  t = clock() - t;
  //printf("spent %fs\n", ((float)t) / CLOCKS_PER_SEC);
  
  // open and create output file
  int outputfd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (outputfd < 0) err_fileopen(outputfile);

  for (i = 0; i < num_of_rec; ++i) {
    int rc;
    rc = write(outputfd, keys[i].record, sizeof(rec_t));
    if (rc != sizeof(rec_t)) err_filewrite(outputfile);
  }
  free(records);
  free(keys);
  return 0;
}
