#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "a1-types.h"
#include "a1-api.h"

#define DEBUG 1
#define PRINT_SUCCESS 1

#ifdef DEBUG
#define A1_ASSERT(c,m) \
        do { \
        if (!(c)) { \
                    printf(m" FAILED on rank %d\n", A1_Rank() ); \
                    fflush(stdout); \
                    sleep(1); \
                    abort(); \
                  } \
        else if (PRINT_SUCCESS) { \
                    printf(m" SUCCEEDED on rank %d\n", A1_Rank() ); \
                    fflush(stdout); \
                  } \
        } \
        while(0);
#else
#define A1_ASSERT(c,m) 
#endif
