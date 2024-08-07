#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ert/util/util.hpp>
#include <ert/util/rng.hpp>
#include <ert/util/mzran.hpp>
#include <ert/util/type_macros.hpp>
#define RNG_TYPE_ID 66154432

#ifdef __cplusplus
extern "C" {
#endif

/**
   The rng structure is a thin structure which wraps a specific Random
   Number Generator (rng) implementation. The rng_struct structure is
   essentially container with function pointers to functions
   manipulating the underlying state.
*/

struct rng_struct {
    UTIL_TYPE_ID_DECLARATION;
    rng_forward_ftype *
        forward; /* Brings the rng forward - returning a random unsigned int value.  This is the fundamental
                                              source of random numbers, and all other random numbers are derived from this through
                                              scaling/shifting/type conversion/... */
    rng_set_state_ftype *
        set_state; /* Takes a char * buffer as input and sets the state of the rng; should set the rng into a default state if arg == NULL. */
    rng_get_state_ftype *get_state;
    rng_alloc_ftype *alloc_state; /* Creates a new instance of this rng. */
    rng_free_ftype *free_state;
    rng_fscanf_ftype *
        fscanf_state; /* Loads the state from a formatted file with (integer representation of) bytes. */
    rng_fprintf_ftype
        *fprintf_state; /* Writes the state as a formatted series of bytes. */
    rng_alg_type type;
    void *
        state; /* The current state - the return value from alloc_state() - passed as parameter to all the function pointers. */
    int state_size; /* How many bytes needed to specify the state of the rng. */
    uint64_t max_value; /* The maximum value this rng can return. */
    double inv_max;
};

UTIL_SAFE_CAST_FUNCTION(rng, RNG_TYPE_ID)
UTIL_IS_INSTANCE_FUNCTION(rng, RNG_TYPE_ID)

rng_type *rng_alloc__(rng_alloc_ftype *alloc_state, rng_free_ftype *free_state,
                      rng_forward_ftype *forward,
                      rng_set_state_ftype *set_state,
                      rng_get_state_ftype *get_state,
                      rng_fscanf_ftype *fscanf_state,
                      rng_fprintf_ftype *fprintf_state, rng_alg_type type,
                      int state_size, uint64_t max_value) {

    rng_type *rng = (rng_type *)util_malloc(sizeof *rng);
    UTIL_TYPE_ID_INIT(rng, RNG_TYPE_ID);
    rng->alloc_state = alloc_state;
    rng->free_state = free_state;
    rng->forward = forward;
    rng->set_state = set_state;
    rng->get_state = get_state;
    rng->fscanf_state = fscanf_state;
    rng->fprintf_state = fprintf_state;

    rng->state_size = state_size;
    rng->max_value = max_value;
    rng->inv_max = 1.0 / max_value;
    rng->type = type;
    rng->state = NULL;

    rng->state = rng->alloc_state();

    rng_forward(rng);
    return rng;
}

/**
   Will initialize the rng with 'random content', using the method
   specified by the @init_mode variable.

   If you have a state which you want to reproduce deterministically you
   should use one of the mzran_set_state() functions.
*/

void rng_init(rng_type *rng, rng_init_mode init_mode) {
    if (init_mode == INIT_DEFAULT)
        rng_set_state(rng, NULL);
    else {
        char *seed_buffer =
            (char *)util_calloc(rng->state_size, sizeof *seed_buffer);

        switch (init_mode) {
        case (INIT_CLOCK): {
            int i;
            for (i = 0; i < rng->state_size; i++)
                seed_buffer[i] = (char)util_clock_seed();
        } break;
        case (INIT_DEV_RANDOM):
            util_fread_dev_random(rng->state_size * sizeof *seed_buffer,
                                  seed_buffer);
            break;
        case (INIT_DEV_URANDOM):
            util_fread_dev_urandom(rng->state_size * sizeof *seed_buffer,
                                   seed_buffer);
            break;
        default:
            util_abort("%s: unrecognized init_code:%d \n", __func__, init_mode);
        }

        rng_set_state(rng, seed_buffer);
        free(seed_buffer);
    }
}

rng_type *rng_alloc(rng_alg_type type, rng_init_mode init_mode) {
    rng_type *rng;
    switch (type) {
    case (MZRAN):
        rng = rng_alloc__(mzran_alloc, mzran_free, mzran_forward,
                          mzran_set_state, mzran_get_state, mzran_fscanf_state,
                          mzran_fprintf_state, type, MZRAN_STATE_SIZE,
                          MZRAN_MAX_VALUE);
        break;
    default:
        util_abort("%s: rng type:%d not recognized \n", __func__, type);
        rng = NULL;
    }

    rng_init(rng, init_mode);
    return rng;
}

int rng_state_size(const rng_type *rng) { return rng->state_size; }

void rng_get_state(const rng_type *rng, char *state) {
    rng->get_state(rng->state, state);
}

void rng_set_state(rng_type *rng, const char *state) {
    rng->set_state(rng->state, state);
}

void rng_free(rng_type *rng) {
    rng->free_state(rng->state);
    free(rng);
}

static void rng_fprintf_state(rng_type *rng, FILE *stream) {
    rng->fprintf_state(rng->state, stream);
}

static void rng_fscanf_state(rng_type *rng, FILE *stream) {
    rng->fscanf_state(rng->state, stream);
}

void rng_load_state(rng_type *rng, const char *filename) {
    FILE *stream = util_fopen(filename, "r");
    rng_fscanf_state(rng, stream);
    fclose(stream);
}

void rng_save_state(rng_type *rng, const char *filename) {
    FILE *stream = util_mkdir_fopen(filename, "w");
    rng_fprintf_state(rng, stream);
    fclose(stream);
}

unsigned int rng_forward(rng_type *rng) { return rng->forward(rng->state); }

double rng_get_double(rng_type *rng) {
    return rng->forward(rng->state) * rng->inv_max;
}

int rng_get_int(rng_type *rng, int max_value) {
    rng_safe_cast(rng);
    return rng->forward(rng->state) % max_value;
}

unsigned int rng_get_max_int(const rng_type *rng) {
    unsigned int MAX = -1;
    if (rng->max_value < MAX) {
        return rng->max_value;
    } else {
        return MAX;
    }
}

#ifdef __cplusplus
}
#endif
