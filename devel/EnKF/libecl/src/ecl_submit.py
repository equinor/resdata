#!/usr/bin/python
import os
import sys

log_file       = "log.txt"
config_file    = "/local/eclipse/macros/ECL.CFG"
license_server = "1700@osl001lic.hda.hydro.com:1700@osl002lic.hda.hydro.com:1700@osl003lic.hda.hydro.com"

max_cpu_sec    = 10000000
max_wall_sec   = 99999999

N_VERSION       	= 2;      # The total number of versions available from this submit program 
VERSION_MIN 	  	= 1;      # The lowest serial number in the version table. 
VERSION_MAX 	  	= 2;      # The highest serial number in the version table. 
DEFAULT_VERSION 	= 1;      # The serial number of the default version. 
NEWEST_VERSION  	= 2;      # The serial number of newest available version. 

DEFAULT_VERSION_INPUT =   0;      # The user input number which will be translated to the default version. 
NEWEST_VERSION_INPUT  = 100;      # The user input number which will be translated to the newest version. 


main_version_table = {"i386":   {1: ("/local/eclipse/Geoquest/2005a_1a/bin/linux/eclipse.exe"         , None , "2005a_1a"),
                                 2: ("/local/eclipse/Geoquest/2006.2/bin/linux/eclipse.exe"           , None , "2006.2"  )},
                      "x86_64": {1: ("/local/eclipse/Geoquest/2005a_1a/bin/linux/x86_64/eclipse.exe"  , None , "2005a_1a"),
                                 2: ("/local/eclipse/Geoquest/2006.2/bin/linux_x86_64/eclipse.exe"    , None , "2006.2")},
                      "ia64":   {1: ("/local/eclipse/Geoquest/2005a_1a/bin/linux/ia64/eclipse.exe"    , None , "2005a_1a"),
                                 2: ("/local/eclipse/Geoquest/2006.2/bin/linux_ia64/eclipse.exe"      , None , "2006.2")}}
                          



def get_version_table():
    cpu = os.uname()[4]
    return main_version_table[cpu]


def print_usage(exe , version_table):
    sys.stderr.write("Python version\n\n")
    sys.stderr.write("%s usage:\n\n   %s  run_path root_name version_id\n\n" % (exe,exe))
    sys.stderr.write("Where run_path is the path to run eclipse from, root_name is the\nname of the ECLIPSE DATA file, without extension, and version_id\n")
    sys.stderr.write("is an integer which indicates which version to use:\n\n")
  
    for key in version_table.keys():
        nr = key
        eclipse_exe = version_table[key][0]
        sys.stderr.write("  %4d: %s \n" %( int(nr) , eclipse_exe))

    sys.stderr.write("\n  %4d: Gives the Hydro default version    (%3d -> %3d) \n" % ( DEFAULT_VERSION_INPUT , DEFAULT_VERSION_INPUT, DEFAULT_VERSION))
    sys.stderr.write("  %4d: Gives the newest version available (%3d -> %3d) \n\n" % ( NEWEST_VERSION_INPUT , NEWEST_VERSION_INPUT, NEWEST_VERSION))
    sys.exit(2)



#################################################################
version_table = get_version_table()
if len(sys.argv) != 4:
    print_usage(sys.argv[0] , version_table)
else:
    run_path   = sys.argv[1]
    base_name  = sys.argv[2]
    version_id = int(sys.argv[3])

    if version_id == DEFAULT_VERSION_INPUT:
        version_id = DEFAULT_VERSION
    elif version_id == NEWEST_VERSION_INPUT:
        version_id = NEWEST_VERSION
    
    version_id = 2
    (eclipse_exe , lib_path , short_name) = version_table[version_id]


    if not os.path.exists(run_path):
        sys.exit("Could not find path:%s " % run_path)

    os.chdir(run_path)
    #fileH = open("cpu" , "w")
    #fileH.write("CPU: %s" % os.uname()[4])
    #fileH.close()
    
    if not os.path.exists("ECL.CFG"):
        os.symlink(config_file, "ECL.CFG");
    
    stream = open("eclipse.stdin" , "w");
    stream.write("%s\n"   % base_name);
    stream.write("%s\n"   % base_name);
    stream.write("%d\n"   % max_cpu_sec);
    stream.write("%d\n\n" % max_wall_sec);
    stream.close()

    stdout_file = base_name + ".stdout"
    stderr_file = base_name + ".stderr"

    fd_stdin  = os.open("eclipse.stdin" , os.O_RDONLY , 0644)
    fd_stdout = os.open(stdout_file     , os.O_WRONLY | os.O_TRUNC | os.O_CREAT , 0644);
    fd_stderr = os.open(stderr_file     , os.O_WRONLY | os.O_TRUNC | os.O_CREAT , 0644);
    
    os.dup2(fd_stdin  , 0)
    os.dup2(fd_stdout , 1)
    os.dup2(fd_stderr , 2)

    os.close(fd_stdin)
    os.close(fd_stdout)
    os.close(fd_stderr)

    env_dict = {"LM_LICENSE_FILE" : license_server , "F_UFMTENDIAN": "big"}
    if lib_path:
        env_dict["LD_LIBRARY_PATH"] = lib_path
    os.execve(eclipse_exe , [eclipse_exe] , env_dict)
