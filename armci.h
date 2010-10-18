#ifndef _ARMCI_H_
#define _ARMCI_H_

#include <mpi.h>

enum  ARMCI_Acc_e { ARMCI_ACC_INT /*     int */, ARMCI_ACC_LNG  /*           long */,
                    ARMCI_ACC_FLT /*   float */, ARMCI_ACC_DBL  /*         double */,
                    ARMCI_ACC_CPL /* complex */, ARMCI_ACC_DCPL /* double complex */ };

int   ARMCI_Init();
int   ARMCI_Init_args(int *argc, char ***argv);
   
int   ARMCI_Finalize();
void  ARMCI_Cleanup();
void  ARMCI_Error(char *msg, int code);

int   ARMCI_Malloc(void **base_ptrs, int size);
int   ARMCI_Free(void *ptr);

void *ARMCI_Malloc_local(int size);
int   ARMCI_Free_local(void *ptr);

void  ARMCI_Barrier();
void  ARMCI_Fence(int proc);
void  ARMCI_AllFence();

void  ARMCI_Access_start(void *ptr); // NEW
void  ARMCI_Access_end(void *ptr);   // NEW

int   ARMCI_Get(void *src, void *dst, int size, int target);
int   ARMCI_Put(void *src, void *dst, int size, int target);
int   ARMCI_Acc(int datatype, void *scale, void *src, void *dst, int bytes, int proc);

int   ARMCI_PutS(void* src_ptr, int src_stride_ar[/*stride_levels*/],
                 void* dst_ptr, int dst_stride_ar[/*stride_levels*/], 
                 int count[/*stride_levels+1*/], int stride_levels, int proc);
int   ARMCI_GetS(void* src_ptr, int src_stride_ar[/*stride_levels*/],
                 void* dst_ptr, int dst_stride_ar[/*stride_levels*/], 
                 int count[/*stride_levels+1*/], int stride_levels, int proc);
int   ARMCI_AccS(int datatype, void *scale,
                 void* src_ptr, int src_stride_ar[/*stride_levels*/],
                 void* dst_ptr, int dst_stride_ar[/*stride_levels*/],
                 int count[/*stride_levels+1*/], int stride_levels, int proc);


/** Non-blocking ops.  MPI-2 forces remote completion on everything so these all
  * currently behave the same as the blocking ops.
  */

typedef int armci_hdl_t;

void  ARMCI_INIT_HANDLE(armci_hdl_t *hdl);
void  ARMCI_SET_AGGREGATE_HANDLE(armci_hdl_t* handle);
void  ARMCI_UNSET_AGGREGATE_HANDLE(armci_hdl_t* handle);

int   ARMCI_NbPut(void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl);
int   ARMCI_NbGet(void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl);
int   ARMCI_NbAcc(int datatype, void *scale, void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl);

int   ARMCI_Wait(armci_hdl_t* hdl);
int   ARMCI_Test(armci_hdl_t* hdl);
int   ARMCI_WaitAll();

int   ARMCI_NbPutS(void* src_ptr, int src_stride_ar[/*stride_levels*/],
                   void* dst_ptr, int dst_stride_ar[/*stride_levels*/], 
                   int count[/*stride_levels+1*/], int stride_levels, int proc, armci_hdl_t *hdl);
int   ARMCI_NbGetS(void* src_ptr, int src_stride_ar[/*stride_levels*/],
                   void* dst_ptr, int dst_stride_ar[/*stride_levels*/], 
                   int count[/*stride_levels+1*/], int stride_levels, int proc, armci_hdl_t *hdl);
int   ARMCI_NbAccS(int datatype, void *scale,
                   void* src_ptr, int src_stride_ar[/*stride_levels*/],
                   void* dst_ptr, int dst_stride_ar[/*stride_levels*/],
                   int count[/*stride_levels+1*/], int stride_levels, int proc, armci_hdl_t *hdl);


/** Generalized I/O Vector operations.
  */

typedef struct {
  void **src_ptr_array;  // Source starting addresses of each data segment.
  void **dst_ptr_array;  // Destination starting addresses of each data segment.
  int    bytes;          // The length of each segment in bytes.
  int    ptr_array_len;  // Number of data segment.
} armci_giov_t;

int ARMCI_PutV(armci_giov_t *iov, int iov_len, int proc);
int ARMCI_GetV(armci_giov_t *iov, int iov_len, int proc);
int ARMCI_AccV(int datatype, void *scale, armci_giov_t *iov, int iov_len, int proc);

int ARMCI_NbPutV(armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle);
int ARMCI_NbGetV(armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle);
int ARMCI_NbAccV(int datatype, void *scale, armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle);


/** Scalar/value operations.
  */

int ARMCI_PutValueInt(int src, void *dst, int proc);
int ARMCI_PutValueLong(long src, void *dst, int proc);
int ARMCI_PutValueFloat(float src, void *dst, int proc);
int ARMCI_PutValueDouble(double src, void *dst, int proc);

int ARMCI_NbPutValueInt(int src, void *dst, int proc, armci_hdl_t *hdl);
int ARMCI_NbPutValueLong(long src, void *dst, int proc, armci_hdl_t *hdl);
int ARMCI_NbPutValueFloat(float src, void *dst, int proc, armci_hdl_t *hdl);
int ARMCI_NbPutValueDouble(double src, void *dst, int proc, armci_hdl_t *hdl);

int    ARMCI_GetValueInt(void *src, int proc);
long   ARMCI_GetValueLong(void *src, int proc);
float  ARMCI_GetValueFloat(void *src, int proc);     
double ARMCI_GetValueDouble(void *src, int proc);     


/** Mutexes
  */

int   ARMCI_Create_mutexes(int count);
int   ARMCI_Destroy_mutexes(void);
void  ARMCI_Lock(int mutex, int proc);
void  ARMCI_Unlock(int mutex, int proc);

/** ARMCI Read-Modify-Write API
  */

enum ARMCI_Rmw_e { ARMCI_FETCH_AND_ADD, ARMCI_FETCH_AND_ADD_LONG, 
                   ARMCI_SWAP, ARMCI_SWAP_LONG };

int ARMCI_Rmw(int op, void *ploc, void *prem, int value, int proc);

/** ARMCI Groups API
  */

typedef struct {
  MPI_Comm  comm;
  int       rank;
  int       size;
} ARMCI_Group;

void ARMCI_Group_create(int grp_size, int *pid_list, ARMCI_Group *group_out);
void ARMCI_Group_create_child(int grp_size, int *pid_list, ARMCI_Group *group_out, ARMCI_Group *group_parent);
void ARMCI_Group_free(ARMCI_Group *group);

int  ARMCI_Group_rank(ARMCI_Group *group, int *rank);
void ARMCI_Group_size(ARMCI_Group *group, int *size);

void ARMCI_Group_set_default(ARMCI_Group *group);
void ARMCI_Group_get_default(ARMCI_Group *group_out);
void ARMCI_Group_get_world(ARMCI_Group *group_out);

int ARMCI_Absolute_id(ARMCI_Group *group,int group_rank);

int ARMCI_Malloc_group(void **ptr_arr, int bytes, ARMCI_Group *group);
int ARMCI_Free_group(void *ptr, ARMCI_Group *group);

#include <armci_msg.h>

#endif /* _ARMCI_H_ */
