#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
/*
  #include <util.h>
*/



typedef struct {
  int          version_nr;
  const char * executable;
  const char * lib_path;
  const char * desc;
} version_type;


#define i386   0
#define X86_64 1
#define IA64   2


/*****************************************************************/

#define N_VERSION      	  	          2       /* The total number of versions available from this submit program */ 
static const int VERSION_MIN 	  	= 1;      /* The lowest serial number in the version table. */
static const int VERSION_MAX 	  	= 2;      /* The highest serial number in the version table. */ 
static const int DEFAULT_VERSION 	= 1;      /* The serial number of the default version. */
static const int NEWEST_VERSION  	= 2;      /* The serial number of newest available version. */

static const int DEFAULT_VERSION_INPUT =   0;    /* The user input number which will be translated to the default version. */ 
static const int NEWEST_VERSION_INPUT  = 100;    /* The user input number which will be translated to the newest version. */


/* 
   This is the table of available versions, each version is one line,
   consisting of four different fields. The first field is an integer
   number which is id of the particular version, this number should be
   matched with the third command line argument given by the user.
   
   The second field is the full path to the executable, the third
   field is the path to the (shared) libraries needed to run the
   executable, this is exported as the environment variable
   LD_LIBRARY_PATH.  If the executable is statically linked, this can
   be NULL. The last field is just a short description of the version,
   this is only used in the diagnostic routine print_usage().
*/

static version_type version_table_i386[N_VERSION] = {{1 , "/local/eclipse/Geoquest/2005a_1a/bin/linux/eclipse.exe" , NULL , "2005a_1a"},
					    	     {2 , "/local/eclipse/Geoquest/2006.2/bin/linux/eclipse.exe"   , NULL , "2006.2"  }};

static version_type version_table_x86_64[N_VERSION] = {{1 , "/local/eclipse/Geoquest/2005a_1a/bin/linux/x86_64/eclipse.exe" , NULL , "2005a_1a"},
					    	       {2 , "/local/eclipse/Geoquest/2006.2/bin/linux_x86_64/eclipse.exe"   , NULL , "2006.2"  }};

static version_type version_table_ia64[N_VERSION] = {{1 , "/local/eclipse/Geoquest/2005a_1a/bin/linux/ia64/eclipse.exe" , NULL , "2005a_1a"},
					    	     {2 , "/local/eclipse/Geoquest/2006.2/bin/linux_ia64/eclipse.exe"   , NULL , "2006.2"  }};


version_type * get_version_table() {
  FILE * stream;
  int cpu_version = -1;
  char word[64];
  system("uname -a > /tmp/uname.out");
  stream = fopen("/tmp/uname.out" , "r");
  
  do {
    int scan_count = fscanf(stream , "%s" , word);
    if (scan_count != 1) {
      fseek(stream , 0L , SEEK_SET);
      
      /*
	Removing util dependancy by commenting this out
	fprintf(stderr,"%s/%s: failed to load string from uname output:\"%s\" - aborting \n",__FILE__ , __func__ , util_fscanf_alloc_line(stream , NULL));
      */
      abort();
    }
    if (strcmp(word , "x86_64") == 0) cpu_version = X86_64;
    if (strcmp(word , "ia64")   == 0) cpu_version = IA64;
    if (strcmp(word , "i686")   == 0) cpu_version = i386;
    if (strcmp(word , "i386")   == 0) cpu_version = i386;
    
  } while (cpu_version < 0);

  if (cpu_version == X86_64)
    return version_table_x86_64;
  else if (cpu_version == IA64)
    return version_table_ia64;
  else if (cpu_version == i386)
    return version_table_i386;
  else {
    fprintf(stderr,"%s/%s - what ??? \n",__FILE__ , __func__);
    abort();
  }
}





/* 
   When adding a new version you must do the following:

   1. Increase the value of N_VERSION.

   2. Increase VERSION_MAX.

   3. Possibly adjust DEFAULT_VERSION / NEWEST_VERSION if the
      new version is either the new default or the now newest
      version.

   4. Add a new line in version_table. Observe that the
      ID numbers in the license table must form a continous
      sequence without 'holes'; i.e. if the last version in
      the table is currently 3, the new version must be 4, and
      can not be 5 or 17...
*/


/*****************************************************************/

static const char * config_file    = "/local/eclipse/macros/ECL.CFG";
static const char * license_server = "1700@osl001lic.hda.hydro.com:1700@osl002lic.hda.hydro.com:1700@osl003lic.hda.hydro.com";


/*****************************************************************/

static const int  max_cpu_sec  = 10000000;
static const int  max_wall_sec = 99999999;

/* End of global variables */
/*****************************************************************/



void print_usage(const char *exe , const version_type * version_table) {
  int i;
  fprintf(stderr,"%s usage:\n\n   %s  run_path root_name version_id\n\n",exe,exe);
  fprintf(stderr,"Where run_path is the path to run eclipse from, root_name is the\nname of the ECLIPSE DATA file, without extension, and version_id\n");
  fprintf(stderr,"is an integer which indicates which version to use:\n\n");
  
  
  for (i=0; i < N_VERSION; i++) 
    fprintf(stderr,"  %4d: %s \n" , version_table[i].version_nr , version_table[i].executable);

  fprintf(stderr,"\n  %4d: Gives the Hydro default version    (%3d -> %3d) \n",DEFAULT_VERSION_INPUT , DEFAULT_VERSION_INPUT, DEFAULT_VERSION);
  fprintf(stderr,"  %4d: Gives the newest version available (%3d -> %3d) \n\n",NEWEST_VERSION_INPUT , NEWEST_VERSION_INPUT, NEWEST_VERSION);
  exit(1);
}


int main(int argc, char **argv) {
  const char * executable;
  const char * lib_path;
  const char * base_name;
  const char * run_path;
  const char * title;
  version_type * version_table;
  char stdout_file[128];
  char stderr_file[128];
  int version_nr;

  version_table = get_version_table();

  if (argc != 4) print_usage(argv[0] , version_table);
  run_path   = argv[1];
  base_name  = argv[2];
  version_nr = atoi(argv[3]);

  if (chdir(run_path) != 0) {
    fprintf(stderr,"%s: failed to change to directory:%s - aborting \n",__FILE__ , run_path);
    abort();
  }
  title = base_name;

  if (version_nr == DEFAULT_VERSION_INPUT)
    version_nr = DEFAULT_VERSION;
  else if (version_nr == NEWEST_VERSION_INPUT)
    version_nr = NEWEST_VERSION;

  if (version_nr < VERSION_MIN || version_nr > VERSION_MAX)
    print_usage(argv[0] , version_table);
  
  executable = version_table[version_nr - VERSION_MIN].executable;
  lib_path   = version_table[version_nr - VERSION_MIN].lib_path;
  symlink(config_file, "ECL.CFG");
  
  {
    FILE *stream = fopen("eclipse.stdin" , "w");
    if (stream == NULL) {
      fprintf(stderr,"Opening: %s/%s failed - aborting \n",run_path , "eclipse.stdin");
      abort();
    }
    fprintf(stream,"%s\n"   , base_name);
    fprintf(stream,"%s\n"   , title);
    fprintf(stream,"%d\n"   , max_cpu_sec);
    fprintf(stream,"%d\n\n" , max_wall_sec);
    fclose(stream);
  }

  strcpy(stdout_file , base_name);  strcat(stdout_file , ".stdout");
  strcpy(stderr_file , base_name);  strcat(stderr_file , ".stderr");
  {
    int fd_stdin  = open("eclipse.stdin" , O_RDONLY , 0644);
    int fd_stdout = open(stdout_file     , O_WRONLY | O_TRUNC | O_CREAT , 0644);
    int fd_stderr = open(stderr_file     , O_WRONLY | O_TRUNC | O_CREAT , 0644);
    dup2(fd_stdin  , 0);
    dup2(fd_stdout , 1);
    dup2(fd_stderr , 2);
    close(fd_stdin);
    close(fd_stdout);
    close(fd_stderr);
  }

  {
    char buffer[1024];
    char *arg[2] , *env[4];
    int i;
    arg[0] = (char *) executable;
    arg[1] = NULL;
    
    
    for (i=0; i < 4; i++)
      env[i] = &buffer[i * 256];

    sprintf(env[0] , "LM_LICENSE_FILE=%s" , license_server);
    sprintf(env[1] , "F_UFMTENDIAN=big");
    if (lib_path != NULL) {
      sprintf(env[2] , "LD_LIBRARY_PATH=%s" , lib_path);
      env[3] = NULL;
    } else 
      env[2] = NULL;
    
    execve(executable , arg , env);
  }
  
  return 0;
}
