#ifndef __SCHED_UTIL_H__
#define __SCHED_UTIL_H__


void sched_util_parse_line(const char * , int * , char *** , int );
void sched_util_free_token_list(int , char **);
void sched_util_parse_file(const char *, int *, char ***);

void   sched_util_fprintf_int(bool ,          int , int , FILE *);
void   sched_util_fprintf_dbl(bool , double , int , int , FILE *);
double sched_util_atof(const char *);
int    sched_util_atoi(const char *);
void   sched_util_fprintf_qst(bool , const char *, int , FILE *);

#endif
