#ifndef __REC_FILE_H__
#define __REC_FILE_H__

typedef struct rec_file_struct rec_file_type;


rec_file_type * rec_file_open(const char *);
void            rec_file_close(rec_file_type *);
bool            rec_file_init_read(rec_file_type *, int);
void            rec_file_init_write(rec_file_type *, int);
void            rec_file_close(rec_file_type *);
void            rec_file_init(const char *, int  , const long int *);
void            rec_file_read(rec_file_type * , void *, int );
void            rec_file_complete_write(const rec_file_type *);
void            rec_file_complete_read(const rec_file_type *);
void            rec_file_write(rec_file_type *  , void *, int );
void            rec_file_write_record(rec_file_type *, int , ...);
void            rec_file_read_record(rec_file_type *, int , ...);
#endif
