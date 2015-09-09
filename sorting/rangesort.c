#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "sort.h"
#include <sys/types.h>
#include <sys/stat.h>

void usage(char *prog) {
  fprintf(stderr, "usage: %s <-i inputfile> <-o outputfile> <-l low value>"
	  "<-h high value>\n", prog);
  exit(1);
}

int rec_cmpfunc(const void *a, const void *b) {
  return (((rec_t*)a)->key - ((rec_t*)b)->key);
}

// Rec representation: 4-byte key + 96-byte record
int main(int argc, char *argv[]) {
  assert(sizeof(rec_t) == 100);

  // arguments
  char *inputfile = "/no/such/file";
  char *outputfile = "/no/such/file";
  int low_value = 0;
  int high_value = 0;

  // input params
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
      low_value  = atoi(optarg);
      break;
    case 'h':
      high_value = atoi(optarg);
      break;
    default:
      usage(argv[0]);
    }
  }
 
  // read input file
  int inputfd = open(inputfile, O_RDONLY);
  if (inputfd < 0) {
    perror("[error] open input file");
    exit(1);
  } 
  // sort
  rec_t r;
  int num_of_rec = 0;
  while (1) {
    int rc;
    rc = read(inputfd, &r, sizeof(rec_t));
    if (rc == 0) break;
    if (rc < 0) {
      perror("[error] read");
      exit(1);
    }
    printf("read in %u\n", r.key);
    ++num_of_rec;
  }
  assert(num_of_rec > 0);
  rec_t records[num_of_rec];
  
  // close and open the file again
  close(inputfd);
  inputfd = open(inputfile, O_RDONLY);
  int i;
  for (i = 0; i < num_of_rec; ++i) {
    int rc;
    rc = read(inputfd, &r, sizeof(rec_t));
    if (rc == 0) break;
    if (rc < 0) {
      perror("[error] read");
      exit(1);
    }
    printf("read again in %u\n", r.key);
    records[i] = r;
    printf("buffer %u\n", records[i].key);
  }
 
  qsort(records, num_of_rec, sizeof(rec_t), rec_cmpfunc);

  // open and create output file
  int outputfd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (outputfd < 0) {
    perror("[error] open output file");
    exit(1);
  }

  for (i = 0; i < num_of_rec; ++i) {
    int rc;
    rc = write(outputfd, &records[i], sizeof(rec_t));
    if (rc != sizeof(rec_t)) {
      perror("[error] write");
      exit(1);
      // remove file
    }
    
  }
  return 0;
}
