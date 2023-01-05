#ifndef ERT_MZRAN_H
#define ERT_MZRAN_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>

typedef struct mzran_struct mzran_type;

#define MZRAN_MAX_VALUE 4294967296
#define MZRAN_STATE_SIZE 16 /* Size of the seed buffer - in bytes. */

void mzran_fscanf_state(void *__rng, FILE *stream);
unsigned int mzran_forward(void *__rng);
void *mzran_alloc(void);
void mzran_set_state(void *__rng, const char *seed_buffer);
void mzran_get_state(void *__rng, char *state_buffer);
double mzran_get_double(mzran_type *rng);
int mzran_get_int(mzran_type *rng, int max);
void mzran_fprintf_state(const void *__rng, FILE *stream);
void mzran_free(void *__rng);

#ifdef __cplusplus
}
#endif
#endif
