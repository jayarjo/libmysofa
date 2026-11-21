#include "../hrtf/tools.h"
#include "tests.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void check(char *filename) {
  struct MYSOFA_HRTF *hrtf;
  int err;

  hrtf = mysofa_load(filename, &err);

  if (!hrtf) {
    CU_FAIL_FATAL("Error reading file.");
    return;
  }

  err = mysofa_check(hrtf);
  CU_ASSERT(err == MYSOFA_OK);

  mysofa_tocartesian(hrtf);

  CU_ASSERT(err == MYSOFA_OK);

  mysofa_free(hrtf);
}

void test_check() {
  check("tests/Pulse.sofa");
  check("tests/MIT_KEMAR_normal_pinna.old.sofa");
}

void test_check_data_sizes() {
  struct MYSOFA_HRTF *hrtf;
  unsigned int elements;
  int err;

  hrtf = mysofa_load("tests/example_dummy_sofa48.sofa", &err);
  if (!hrtf) {
    CU_FAIL_FATAL("Error reading file.");
    return;
  }

  elements = hrtf->DataIR.elements;
  hrtf->DataIR.elements--;
  CU_ASSERT(mysofa_check(hrtf) == MYSOFA_INVALID_FORMAT);
  hrtf->DataIR.elements = elements;

  if (hrtf->DataDelay.values) {
    elements = hrtf->DataDelay.elements;
    hrtf->DataDelay.elements--;
    CU_ASSERT(mysofa_check(hrtf) == MYSOFA_INVALID_FORMAT);
    hrtf->DataDelay.elements = elements;
  }

  hrtf->N = 0;
  CU_ASSERT(mysofa_check(hrtf) == MYSOFA_INVALID_DIMENSIONS);

  mysofa_free(hrtf);
}

/* Each file declares a size that it cannot hold. The first four are
 * tests/testfile.sofa with one declared size raised: an attribute name in a
 * fractal heap, a chunk, and two chunked datasets that would inflate to more
 * than the whole file can. fail-eof-past-file is
 * tests/example_dummy_sofa48.sofa (superblock version 0) cut off where the
 * data of Data.IR begins: its superblock still declares the whole length.
 * fail-chunk-inflates-past-chunk is tests/Pulse.sofa with a chunk dimension
 * raised, so that one chunk of 11 compressed bytes would inflate to 96 MB.
 * fail-dataset-size-overflows is tests/Pulse.sofa with two dimensions and an
 * element size raised, so that the dataset's size wraps 64 bits to 118 MB.
 * The load must refuse each file before it allocates a buffer of that size;
 * under an allocation limit of 64 MB, an allocation of that size returns
 * MYSOFA_NO_MEMORY instead. */
void test_check_sizes_past_end() {
  static const char *files[] = {"tests/fail-heap-name-past-end.sofa",
                                "tests/fail-chunk-past-end.sofa",
                                "tests/fail-chunk-inflates-past-file.sofa",
                                "tests/fail-dataset-inflates-past-file.sofa",
                                "tests/fail-eof-past-file.sofa",
                                "tests/fail-chunk-inflates-past-chunk.sofa",
                                "tests/fail-dataset-size-overflows.sofa"};
  struct MYSOFA_HRTF *hrtf;
  unsigned int i;
  int err;

  for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
    err = MYSOFA_OK;
    hrtf = mysofa_load(files[i], &err);
    CU_ASSERT(hrtf == NULL);
    CU_ASSERT(err == MYSOFA_INVALID_FORMAT);
    mysofa_free(hrtf);
  }
}

/* Each file is tests/testfile.sofa with one size in a fractal heap's header
 * set to 0. Each of these sizes goes into a logarithm: the load must refuse
 * the file, not take log2(0). The file with a starting block size of 0 used
 * to load. */
void test_check_zero_heap_sizes() {
  static const char *files[] = {
      "tests/fail-heap-zero-starting-block-size.sofa",
      "tests/fail-heap-zero-maximum-direct-block-size.sofa",
      "tests/fail-heap-zero-maximum-heap-size.sofa",
      "tests/fail-heap-zero-maximum-size.sofa"};
  struct MYSOFA_HRTF *hrtf;
  unsigned int i;
  int err;

  for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
    err = MYSOFA_OK;
    hrtf = mysofa_load(files[i], &err);
    CU_ASSERT(hrtf == NULL);
    CU_ASSERT(err == MYSOFA_INVALID_FORMAT);
    mysofa_free(hrtf);
  }
}

/* The file's Conventions attribute is "SOFX". The load refuses it after the
 * file's attributes have moved to the new profile, and must free them with
 * it: built with -DADDRESS_SANITIZE=ON, LeakSanitizer reports the leak. */
void test_check_conventions_not_sofa() {
  struct MYSOFA_HRTF *hrtf;
  int err = MYSOFA_OK;

  hrtf = mysofa_load("tests/fail-conventions-not-sofa.sofa", &err);
  CU_ASSERT(hrtf == NULL);
  CU_ASSERT(err == MYSOFA_INVALID_FORMAT);
  mysofa_free(hrtf);
}
