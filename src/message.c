/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include <debug.h>
#include <armci.h>
#include <armci_internals.h>

/** Query process rank from messaging (MPI) layer.
  */
int armci_msg_me(void) {
  int me;
  MPI_Comm_rank(ARMCI_GROUP_WORLD.comm, &me);
  return me;
}


/** Query number of processes.
  */
int armci_msg_nproc(void) {
  int nproc;
  MPI_Comm_size(ARMCI_GROUP_WORLD.comm, &nproc);
  return nproc;
}


/** Abort the application.
  *
  * @param[in] code Exit error code
  */
void armci_msg_abort(int code) {
  MPI_Abort(ARMCI_GROUP_WORLD.comm, code);
}


/** Get the wall clock time.
  *
  * @return Wall clock time
  */
double armci_timer(void) {
  return MPI_Wtime();
}


/** Broadcast a message.  Collective.
  *
  * @param[in] buffer Source buffer on root, destination elsewhere.
  * @param[in] len    Length of the message in bytes.
  * @param[in] root   Rank of the root process.
  */
void armci_msg_bcast(void *buffer, int len, int root) {
  MPI_Bcast(buffer, len, MPI_BYTE, root, ARMCI_GROUP_WORLD.comm);
}


/** Broadcast a message.  Collective.
  *
  * @param[in] buffer Source buffer on root, destination elsewhere.
  * @param[in] len    Length of the message in bytes.
  * @param[in] root   Rank of the root process.
  */
void armci_msg_brdcst(void *buffer, int len, int root) {
  MPI_Bcast(buffer, len, MPI_BYTE, root, ARMCI_GROUP_WORLD.comm);
}


/** Broadcast a message on the given scope.  Collective.
  *
  * @param[in] scope  Scope for the broadcast
  * @param[in] buffer Source buffer on root, destination elsewhere.
  * @param[in] len    Length of the message in bytes.
  * @param[in] root   Rank of the root process.
  */
void armci_msg_bcast_scope(int scope, void *buffer, int len, int root) {
  armci_msg_group_bcast_scope(scope, buffer, len, root, &ARMCI_GROUP_WORLD);
}


/** Barrier from the messaging layer.
  */
void armci_msg_barrier(void) {
  MPI_Barrier(ARMCI_GROUP_WORLD.comm);
}


/** Message barrier on a group.
  *
  * @param[in] group Group on which to perform barrier
  */
void armci_msg_group_barrier(ARMCI_Group *group) {
  MPI_Barrier(group->comm);
}


/** Broadcast on a group. Collective.
  *
  * @param[in]    scope ARMCI scope
  * @param[inout] buf   Input on the root, output on all other processes
  * @param[in]    len   Number of bytes in the message
  * @param[in]    abs_root Absolute rank of the process at the root of the broadcast
  * @param[in]    group ARMCI group on which to perform communication
  */
void armci_msg_group_bcast_scope(int scope, void *buf, int len, int abs_root, ARMCI_Group *group) {
  int grp_root;

  if (scope == SCOPE_ALL || scope == SCOPE_MASTERS) {
    grp_root = ARMCII_Translate_absolute_to_group(group->comm, abs_root);
    ARMCII_Assert(grp_root >= 0 && grp_root < group->size);

    MPI_Bcast(buf, len, MPI_BYTE, grp_root, group->comm);

  } else /* SCOPE_NODE */ {
    // grp_root = ARMCII_Translate_absolute_to_group(MPI_COMM_SELF, abs_root);
    // ARMCII_Assert(grp_root >= 0 && grp_root < group->size);
    grp_root = 0;

    MPI_Bcast(buf, len, MPI_BYTE, grp_root, MPI_COMM_SELF);
  }
}


/** Send a two-sided message.
  *
  * @param[in] tag    Message tag (must match on sender and receiver)
  * @param[in] buf    Buffer containing the message
  * @param[in] nbytes Length of the message in bytes
  * @param[in] dest   Destination process id
  */
void armci_msg_snd(int tag, void *buf, int nbytes, int dest) {
  MPI_Send(buf, nbytes, MPI_BYTE, dest, tag, ARMCI_GROUP_WORLD.comm);
}


/** Receive a two-sided message.
  *
  * @param[in]  tag    Message tag (must match on sender and receiver)
  * @param[in]  buf    Buffer containing the message
  * @param[in]  nbytes_buf Size of the buffer in bytes
  * @param[out] nbytes_msg Length of the message received in bytes (NULL to ignore)
  * @param[in]  src    Source process id
  */
void armci_msg_rcv(int tag, void *buf, int nbytes_buf, int *nbytes_msg, int src) {
  MPI_Status status;
  MPI_Recv(buf, nbytes_buf, MPI_BYTE, src, tag, ARMCI_GROUP_WORLD.comm, &status);

  if (nbytes_msg != NULL)
    MPI_Get_count(&status, MPI_BYTE, nbytes_msg);
}


/** Receive a two-sided message from any source.
  *
  * @param[in]  tag    Message tag (must match on sender and receiver)
  * @param[in]  buf    Buffer containing the message
  * @param[in]  nbytes_buf Size of the buffer in bytes
  * @param[out] nbytes_msg Length of the message received in bytes (NULL to ignore)
  * @return            Rank of the message source
  */
int armci_msg_rcvany(int tag, void *buf, int nbytes_buf, int *nbytes_msg) {
  MPI_Status status;
  MPI_Recv(buf, nbytes_buf, MPI_BYTE, MPI_ANY_SOURCE, tag, ARMCI_GROUP_WORLD.comm, &status);

  if (nbytes_msg != NULL)
    MPI_Get_count(&status, MPI_BYTE, nbytes_msg);

  return status.MPI_SOURCE;
}


void armci_msg_reduce(void *x, int n, char *op, int type) {
  armci_msg_reduce_scope(SCOPE_ALL, x, n, op, type);
}


void armci_msg_reduce_scope(int scope, void *x, int n, char *op, int type) {
  ARMCII_Error("unimplemented"); // TODO
}


/** Map process IDs onto a binary tree.
  *
  * @param[in]  scope Scope of processes involved
  * @param[out] root  Process id of the root
  * @param[out] up    Process id of my parent
  * @param[out] left  Process id of my left child
  * @param[out] right Process if of my right child
  */
void armci_msg_bintree(int scope, int *root, int *up, int *left, int *right) {
  int me, nproc;

  if (scope == SCOPE_NODE) {
    *root  = 0;
    *left  = -1;
    *right = -1;
   
    return;
  }

  me    = armci_msg_me();
  nproc = armci_msg_nproc();

  *root = 0;
  *up   =  (me == 0) ? -1 : (me - 1) / 2;

  *left = 2*me + 1;
  if (*left >= nproc) *left = -1;

  *right = 2*me + 2;
  if (*right >= nproc) *right = -1;
}


/** I have no idea what this does.  It's needed by GA.  FIXME.
  */
void armci_msg_sel(void *x, int n, char *op, int type, int contribute) {
  armci_msg_sel_scope(SCOPE_ALL, x, n, op, type, contribute);
}
