#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "debug.h"

#include "armci.h"
#include "mem_region.h"


/** Declare the start of a local access epoch.  This allows direct access to
  * data in local memory.  (TODO: Does MPI-2 actually allow this?)
  *
  * @param[in] ptr Pointer to the allocation that will be accessed directly 
  */
void ARMCI_Access_start(void *ptr) {
  int           me, grp_rank;
  mem_region_t *mreg;
  MPI_Group     grp;

  MPI_Comm_rank(ARMCI_GROUP_WORLD.comm, &me);

  mreg = mem_region_lookup(ptr, me);
  assert(mreg != NULL);

  MPI_Win_get_group(mreg->window, &grp);
  MPI_Group_rank(grp, &grp_rank);
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, grp_rank, 0, mreg->window);
  MPI_Group_free(&grp);
}


/** Declare the end of a local access epoch.
  *
  * TODO: Should we allow multiple accesses at once?
  *
  * @param[in] ptr Pointer to the allocation that was accessed directly 
  */
void ARMCI_Access_end(void *ptr) {
  int           me, grp_rank;
  mem_region_t *mreg;
  MPI_Group     grp;

  MPI_Comm_rank(ARMCI_GROUP_WORLD.comm, &me);

  mreg = mem_region_lookup(ptr, me);
  assert(mreg != NULL);

  MPI_Win_get_group(mreg->window, &grp);
  MPI_Group_rank(grp, &grp_rank);
  MPI_Win_unlock(grp_rank, mreg->window);
  MPI_Group_free(&grp);
}


/** One-sided get operation.
  *
  * @param[in] src    Source address (remote)
  * @param[in] dst    Destination address (local)
  * @param[in] size   Number of bytes to transfer
  * @param[in] target Process id to target
  * @return           0 on success, non-zero on failure
  */
int ARMCI_Get(void *src, void *dst, int size, int target) {
  int disp;
  mem_region_t *mreg;

  mreg = mem_region_lookup(src, target);
  assert(mreg != NULL);

  // Calculate displacement from beginning of the window
  disp = (int) ((u_int8_t*)src - (u_int8_t*)mreg->slices[target].base);

  assert(disp >= 0 && disp < mreg->slices[target].size);
  assert(src >= mreg->slices[target].base);
  assert((u_int8_t*)src + size <= (u_int8_t*)mreg->slices[target].base + mreg->slices[target].size);

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, mreg->window);
  MPI_Get(dst, size, MPI_BYTE, target, disp, size, MPI_BYTE, mreg->window);
  MPI_Win_unlock(target, mreg->window);

  return 0;
}


/** One-sided put operation.
  *
  * @param[in] src    Source address (remote)
  * @param[in] dst    Destination address (local)
  * @param[in] size   Number of bytes to transfer
  * @param[in] target Process id to target
  * @return           0 on success, non-zero on failure
  */
int ARMCI_Put(void *src, void *dst, int size, int target) {
  int disp;
  mem_region_t *mreg;

  mreg = mem_region_lookup(dst, target);
  assert(mreg != NULL);

  // Calculate displacement from beginning of the window
  disp = (int) ((u_int8_t*)dst - (u_int8_t*)mreg->slices[target].base);

  assert(disp >= 0 && disp < mreg->slices[target].size);
  assert(dst >= mreg->slices[target].base);
  assert((u_int8_t*)dst + size <= (u_int8_t*)mreg->slices[target].base + mreg->slices[target].size);

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, mreg->window);
  MPI_Put(src, size, MPI_BYTE, target, disp, size, MPI_BYTE, mreg->window);
  MPI_Win_unlock(target, mreg->window);

  return 0;
}


/** One-sided accumulate operation.
  *
  * @param[in] datatype ARMCI data type for the accumulate operation (see armci.h)
  * @param[in] scale    Pointer for a scalar of type datatype that will be used to
  *                     scale values in the source buffer
  * @param[in] src      Source address (remote)
  * @param[in] dst      Destination address (local)
  * @param[in] bytes    Number of bytes to transfer
  * @param[in] proc     Process id to target
  * @return             0 on success, non-zero on failure
  */
int ARMCI_Acc(int datatype, void *scale, void* src, void* dst, int bytes, int proc) {
  void *scaled_data = NULL;
  void *src_data;
  int   count, type_size, i, disp;
  MPI_Datatype type;
  mem_region_t *mreg;

  switch (datatype) {
    case ARMCI_ACC_INT:
      MPI_Type_size(MPI_INT, &type_size);
      type = MPI_INT;
      count= bytes/type_size;

      if (*((int*)scale) == 1)
        break;
      else {
        int *src_i = (int*) src;
        int *scl_i = malloc(bytes);
        const int s = *((int*) scale);
        scaled_data = scl_i;
        for (i = 0; i < count; i++)
          scl_i[i] = src_i[i]*s;
      }
      break;

    case ARMCI_ACC_LNG:
      MPI_Type_size(MPI_LONG, &type_size);
      type = MPI_LONG;
      count= bytes/type_size;

      if (*((long*)scale) == 1)
        break;
      else {
        long *src_l = (long*) src;
        long *scl_l = malloc(bytes);
        const long s = *((long*) scale);
        scaled_data = scl_l;
        for (i = 0; i < count; i++)
          scl_l[i] = src_l[i]*s;
      }
      break;

    case ARMCI_ACC_FLT:
      MPI_Type_size(MPI_FLOAT, &type_size);
      type = MPI_FLOAT;
      count= bytes/type_size;

      if (*((float*)scale) == 1.0)
        break;
      else {
        float *src_f = (float*) src;
        float *scl_f = malloc(bytes);
        const float s = *((float*) scale);
        scaled_data = scl_f;
        for (i = 0; i < count; i++)
          scl_f[i] = src_f[i]*s;
      }
      break;

    case ARMCI_ACC_DBL:
      MPI_Type_size(MPI_DOUBLE, &type_size);
      type = MPI_DOUBLE;
      count= bytes/type_size;

      if (*((double*)scale) == 1.0)
        break;
      else {
        double *src_d = (double*) src;
        double *scl_d = malloc(bytes);
        const double s = *((double*) scale);
        scaled_data = scl_d;
        for (i = 0; i < count; i++)
          scl_d[i] = src_d[i]*s;
      }
      break;

    default:
      ARMCI_Error("ARMCI_Acc() unsupported operation", 100);
  }

  if (scaled_data)
    src_data = scaled_data;
  else
    src_data = src;

  // Calculate displacement from window's base address
  mreg = mem_region_lookup(dst, proc);
  assert(mreg != NULL);

  assert(bytes % type_size == 0);

  disp = (int) ((u_int8_t*)dst - (u_int8_t*)mreg->slices[proc].base);

  assert(disp >= 0 && disp < mreg->slices[proc].size);
  assert(dst >= mreg->slices[proc].base);
  assert((u_int8_t*)dst + bytes <= (u_int8_t*)mreg->slices[proc].base + mreg->slices[proc].size);

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, mreg->window);
  MPI_Accumulate(src_data, count, type, proc, disp, count, type, MPI_SUM, mreg->window);
  MPI_Win_unlock(proc, mreg->window);

  if (scaled_data != NULL) 
    free(scaled_data);

  return 0;
}
