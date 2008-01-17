#ifndef __ENKF_FS_H__
#define __ENKF_FS_H__

typedef struct enkf_fs_struct enkf_fs_type;

enkf_fs_type * enkf_fs_alloc(void);
void           enkf_fs_free(enkf_fs_type *);

#endif
