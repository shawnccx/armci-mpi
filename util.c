#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "debug.h"

#include "armci.h"
#include "armci-internals.h"

int ARMCI_Init() {
  MPI_Comm_dup(MPI_COMM_WORLD, &ARMCI_GROUP_WORLD.comm);
  MPI_Comm_rank(ARMCI_GROUP_WORLD.comm, &ARMCI_GROUP_WORLD.rank);
  MPI_Comm_size(ARMCI_GROUP_WORLD.comm, &ARMCI_GROUP_WORLD.size);
  return 0;
}

int ARMCI_Init_args(int *argc, char ***argv) {
  return 0;
}

int ARMCI_Finalize() {
  return 0;
}

void ARMCI_Cleanup() {
  return;
}

void ARMCI_Error(char *msg, int code) {
  fprintf(stderr, "ARMCI_Error: %s\n", msg);
  MPI_Abort(MPI_COMM_WORLD, code);
}

void ARMCI_Barrier() {
  ARMCI_AllFence();
  MPI_Barrier(MPI_COMM_WORLD);
}

/** \brief Wait for remote completion on one-sided operations targeting
  * process proc.
  *
  * In MPI-2, this is a no-op since get/put/acc already guarantee remote
  * completion.
  *
  * @param[in] proc Process to target
  */
void ARMCI_Fence(int proc) {
  return;
}

/** \brief Wait for remote completion on all one-sided operations.
  *
  * In MPI-2, this is a no-op since get/put/acc already guarantee remote
  * completion.
  */
void ARMCI_AllFence() {
  return;
}
