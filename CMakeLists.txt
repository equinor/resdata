cmake_minimum_required(VERSION 3.16)
project(resdata C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(TestBigEndian)

option(BUILD_TESTS "Should the tests be built" ON)
option(BUILD_APPLICATIONS "Should we build small utility applications" OFF)
option(BUILD_RD_SUMMARY "Build the commandline application rd_summary" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ERT_USE_OPENMP "Use OpenMP" OFF)
option(RST_DOC "Build RST documentation" OFF)
option(USE_RPATH "Don't strip RPATH from libraries and binaries" OFF)
option(USE_CONAN "Use Conan (https://conan.io) for package management" ON)

# -----------------------------------------------------------------
# Set default CMAKE_BUILD_TYPE
# -----------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "CMake build type" FORCE)
  message(
    STATUS
      "Setting CMake build type to ${CMAKE_BUILD_TYPE} as none was specified")

  # Set possible values for use in cmake-gui
  set(CACHE
      CMAKE_BUILD_TYPE
      PROPERTY
      STRINGS
      "Debug"
      "Release"
      "MinSizeRel"
      "RelWithDebInfo")
endif()

# -----------------------------------------------------------------
# Conan: C++ package manager
# https://docs.conan.io/en/latest/howtos/cmake_launch.html
# -----------------------------------------------------------------

if(USE_CONAN)
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(
      STATUS
        "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(
      DOWNLOAD
      "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
      "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()

  include(${CMAKE_BINARY_DIR}/conan.cmake)

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # set(conan_opts ...)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(conan_opts backward-cpp:stack_details=backtrace_symbol)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # set(conan_opts ...)
  else()
    message(
      WARNING
        "Unknown platform. Conan packages may not be configures correctly.")
  endif()

  conan_cmake_run(
    # Packages
    REQUIRES
    backward-cpp/1.6
    catch2/2.13.9
    fmt/8.0.1
    # Options
    OPTIONS
    catch2:with_main=True
    ${conan_opts}
    # Force cppstd to be the same as this CMakeLists.txt's
    SETTINGS
    compiler.cppstd=${CMAKE_CXX_STANDARD}
    # Build from source if there are no pre-compiled binaries
    BUILD
    missing
    GENERATORS
    cmake_find_package)
endif(USE_CONAN)

# -----------------------------------------------------------------

set(RD_VERSION_MAJOR 0)
set(RD_VERSION_MINOR 0)
set(RD_VERSION_MICRO "not-available")

# If the micro version is not integer, that should be interpreted as a
# development version leading towards version MAJOR.MINOR.0

execute_process(COMMAND date "+%Y-%m-%d %H:%M:%S" OUTPUT_VARIABLE RD_BUILD_TIME)
string(STRIP "${RD_BUILD_TIME}" RD_BUILD_TIME)

if(RD_VERSION)
  # Have we been provided with an explicitly-set version?
  string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.(.+)$" "\\1" RD_VERSION_MAJOR
                       "${RD_VERSION}")
  string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.(.+)$" "\\2" RD_VERSION_MINOR
                       "${RD_VERSION}")
  string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.(.+)$" "\\3" RD_VERSION_MICRO
                       "${RD_VERSION}")
else()
  # Otherwise try to discover it via git
  find_package(Git)

  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} "--git-dir=${CMAKE_SOURCE_DIR}/.git" status
      RESULT_VARIABLE RESULT
      OUTPUT_QUIET ERROR_QUIET)

    if(NOT "${RESULT}" STREQUAL 0)
      set(GIT_FOUND OFF)
    endif()
  endif()

  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} "--git-dir=${CMAKE_SOURCE_DIR}/.git" rev-parse
              HEAD
      OUTPUT_VARIABLE GIT_COMMIT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(
      COMMAND ${GIT_EXECUTABLE} "--git-dir=${CMAKE_SOURCE_DIR}/.git" rev-parse
              --short HEAD
      OUTPUT_VARIABLE GIT_COMMIT_SHORT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(
      COMMAND ${GIT_EXECUTABLE} "--git-dir=${CMAKE_SOURCE_DIR}/.git" describe
              --tags
      OUTPUT_VARIABLE GIT_TAG
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.(.+)$" "\\1" RD_VERSION_MAJOR
                         "${GIT_TAG}")
    string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.(.+)$" "\\2" RD_VERSION_MINOR
                         "${GIT_TAG}")
    string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.(.+)$" "\\3" RD_VERSION_MICRO
                         "${GIT_TAG}")
  else()
    set(GIT_COMMIT "unknown (git not found!)")
    set(GIT_COMMIT_SHORT "unknown (git not found!)")
    message(
      WARNING "Git not found. Build will not contain correct version info.")
  endif()
endif()

message(
  STATUS
    "resdata version: ${RD_VERSION_MAJOR}.${RD_VERSION_MINOR}.${RD_VERSION_MICRO}"
)

# -----------------------------------------------------------------

set(EQUINOR_TESTDATA_ROOT
    ""
    CACHE PATH "Root to Equinor internal testdata")
if(EXISTS ${EQUINOR_TESTDATA_ROOT})
  set(LINK "${CMAKE_CURRENT_SOURCE_DIR}/test-data/Equinor")
  if(EXISTS ${LINK})
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove "${LINK}")
  endif()

  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                          "${EQUINOR_TESTDATA_ROOT}" "${LINK}")
  message(STATUS "Linking testdata: ${LINK} -> ${EQUINOR_TESTDATA_ROOT}")

  set(_equinor_test_data ${CMAKE_SOURCE_DIR}/test-data/Equinor)
  set(_resdatapath ${_equinor_test_data}/ECLIPSE)
  set(_geopath ${_equinor_test_data}/Geometry)
endif()
set(_local_test_data "${CMAKE_CURRENT_SOURCE_DIR}/test-data/local")
set(_local_resdatapath ${_local_test_data}/ECLIPSE)

# output libs to some lib/ path for testing
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

if(USE_RPATH)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

enable_testing()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  message(STATUS "Found Linux")
  set(ERT_LINUX TRUE)
  add_definitions(-DERT_LINUX)
  set(ERT_BINARY_POSTFIX .${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message(STATUS "Found Darwin")
  set(ERT_LINUX TRUE)
  set(ERT_MAC TRUE)
  add_definitions(-DERT_LINUX)
  set(ERT_BINARY_POSTFIX .${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  message(STATUS "Found Windows")
  set(ERT_WINDOWS TRUE)
  add_definitions(-DERT_WINDOWS)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(
    STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE
        STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

# Treat warnings as errors if not on Windows
if(NOT ERT_WINDOWS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wno-unknown-pragmas ")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wfatal-errors -Wall -Wno-unknown-pragmas -Wno-unused-result -Wno-unused-parameter"
  )
endif()

if(MSVC)
  add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /wd4996")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules
     "${CMAKE_CURRENT_BINARY_DIR}")
find_package(CXX11Features)

# -----------------------------------------------------------------

find_package(fmt REQUIRED)
if(USE_CONAN)
  find_package(Backward REQUIRED)
else()
  find_path(Backward_INCLUDES backward.hpp REQUIRED)
endif(USE_CONAN)

# create targets for (required and optional) dependencies to link to

find_package(OpenMP)
if(NOT OPENMP_FOUND)
  message(STATUS "OpenMP package not found")
  set(ERT_USE_OPENMP OFF)
endif()

if(NOT ERT_USE_OPENMP)
  message(STATUS "OpenMP disabled")
endif()

# when cmake can be upgraded to >= 3.0, dependencies should be implemented
# either via target-providing FindLib.cmake:
# ~~~
# find_package(Threads)
# target_link_libraries(lib Threads::Threads)
# ~~~
#
# or via INTERFACE libraries: add_library(m INTERFACE) find_library(M_LIBRARY
# NAMES m) if (M_LIBRARY) target_link_libraries(m ${M_LIBRARY}) endif ()
# target_link_libraries(m ${M_LIBRARY})
#
# These targets can then be exported along with targets provided by this
# project. Linking against cmake interface targets will handle include
# directories better than raw names given to target_link_libraries

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
if(CMAKE_USE_PTHREADS_INIT)
  set(HAVE_PTHREAD TRUE)

  set(pthread "${CMAKE_THREAD_LIBS_INIT}")

  if(THREADS_HAVE_PTHREAD_ARG)
    set(pthreadarg "-pthread")
  endif()
endif()

set(dl "${CMAKE_DL_LIBS}")

find_library(M_LIBRARY NAMES m)
if(M_LIBRARY)
  set(m m)
endif()

find_package(ZLIB)
if(ZLIB_FOUND)
  set(ERT_HAVE_ZLIB ON)
  set(zlib "${ZLIB_LIBRARIES}")
endif()

find_library(SHLWAPI_LIBRARY NAMES Shlwapi)
if(SHLWAPI_LIBRARY)
  set(shlwapi ${SHLWAPI_LIBRARY})
endif()

# -----------------------------------------------------------------

# feature tests
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)

check_function_exists(access HAVE_POSIX_ACCESS)
check_function_exists(_access HAVE_WINDOWS__ACCESS)
check_function_exists(chdir HAVE_POSIX_CHDIR)
check_function_exists(_chdir HAVE_WINDOWS_CHDIR)
check_function_exists(chmod HAVE_CHMOD)
check_function_exists(fnmatch HAVE_FNMATCH)
check_function_exists(fork HAVE_FORK)
check_function_exists(fseeko HAVE_FSEEKO)
check_function_exists(fsync HAVE_FSYNC)
check_function_exists(ftruncate HAVE_FTRUNCATE)
check_function_exists(getcwd HAVE_POSIX_GETCWD)
check_function_exists(_getcwd HAVE_WINDOWS_GETCWD)
check_function_exists(getpwuid HAVE_GETPWUID)
check_function_exists(GetTempPath HAVE_WINDOWS_GET_TEMP_PATH)
check_function_exists(getuid ERT_HAVE_GETUID)
check_function_exists(glob ERT_HAVE_GLOB)
check_function_exists(gmtime_r HAVE_GMTIME_R)
check_function_exists(localtime_r HAVE_LOCALTIME_R)
check_function_exists(lockf ERT_HAVE_LOCKF)
check_function_exists(mkdir HAVE_POSIX_MKDIR)
check_function_exists(_mkdir HAVE_WINDOWS_MKDIR)
check_function_exists(opendir ERT_HAVE_OPENDIR)
check_function_exists(posix_spawn ERT_HAVE_SPAWN)
check_function_exists(readlinkat ERT_HAVE_READLINKAT)
check_function_exists(realpath HAVE_REALPATH)
check_function_exists(regexec ERT_HAVE_REGEXP)
check_function_exists(round HAVE_ROUND)
check_function_exists(symlink ERT_HAVE_SYMLINK)
check_function_exists(timegm HAVE_TIMEGM)
check_function_exists(usleep HAVE__USLEEP)
check_function_exists(unlink HAVE_POSIX_UNLINK)
check_function_exists(_unlink HAVE_WINDOWS_UNLINK)

check_symbol_exists(_tzname time.h HAVE_WINDOWS_TZNAME)
check_symbol_exists(tzname time.h HAVE_TZNAME)

check_include_file(execinfo.h HAVE_EXECINFO)
check_include_file(getopt.h ERT_HAVE_GETOPT)
check_include_file(unistd.h ERT_HAVE_UNISTD)

test_big_endian(BIG_ENDIAN)

check_type_size(time_t SIZE_OF_TIME_T)
if(${SIZE_OF_TIME_T} EQUAL 8)
  try_run(RUN_RESULT COMPILE_RESULT ${CMAKE_BINARY_DIR}
          ${PROJECT_SOURCE_DIR}/cmake/Tests/test_mktime_before1970.c)
  if(${COMPILE_RESULT})
    if(${RUN_RESULT} EQUAL 0)
      set(ERT_TIME_T_64BIT_ACCEPT_PRE1970 ON)
    endif()
  endif()
endif()

try_compile(HAVE_VA_COPY ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/cmake/Tests/test_va_copy.c)
try_compile(HAVE_SIGBUS ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/cmake/Tests/test_have_sigbus.c)
try_compile(HAVE_PID_T ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/cmake/Tests/test_pid_t.c)
try_compile(HAVE_MODE_T ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/cmake/Tests/test_mode_t.c)
try_compile(ERT_HAVE_ISFINITE ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/cmake/Tests/test_isfinite.c)
try_compile(HAVE_SIGHANDLER_T ${PROJECT_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/cmake/Tests/test_have_sighandler.c)

if(ERT_HAVE_READLINKAT)
  try_compile(ERT_HAVE_READLINKAT_DECLARATION ${CMAKE_BINARY_DIR}
              ${PROJECT_SOURCE_DIR}/cmake/Tests/test_have_readlinkat_decl.c)
endif()

set(BUILD_CXX ON)
try_compile(HAVE_CXX_SHARED_PTR ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/cmake/Tests/test_shared_ptr.cpp)
if(NOT HAVE_CXX_SHARED_PTR)
  set(BUILD_CXX OFF)
endif()

if(ERT_WINDOWS)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ERT_WINDOWS_LFS ON)
  endif()
endif()

find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  set(GIT_COMMIT "unknown (git not found!)")
  set(GIT_COMMIT_SHORT "unknown (git not found!)")
  message(WARNING "Git not found. Build will not contain git revision info.")
endif()

if(ERT_WINDOWS)
  execute_process(COMMAND cmd.exe /c "echo %date% %time%"
                  OUTPUT_VARIABLE BUILD_TIME)
else() # Linux or Darwin
  execute_process(COMMAND date "+%Y-%m-%d %H:%M:%S" OUTPUT_VARIABLE BUILD_TIME)
endif()
string(STRIP "${BUILD_TIME}" BUILD_TIME)

# -----------------------------------------------------------------

set(INSTALL_GROUP
    ""
    CACHE STRING "Group to install as - blank to install as current group")

if(MSVC)
  add_definitions(-D__func__="\\"????\\"")
endif()

if(ERT_LINUX)
  add_definitions(-DHAVE_PROC)
endif()

if(BUILD_TESTS)
  find_package(Catch2 REQUIRED)
endif()

add_subdirectory(lib)
add_subdirectory(applications)
add_subdirectory(bin)

if(RST_DOC)
  add_subdirectory(python/docs)
endif()

if(NOT SKBUILD)
  # Avoid installing when calling from python setup.py
  install(EXPORT resdata-config DESTINATION share/cmake/resdata)
  export(TARGETS resdata FILE resdataConfig.cmake)
  export(PACKAGE resdata)
endif()
