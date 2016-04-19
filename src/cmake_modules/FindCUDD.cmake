# - Find the CUDD BDD package.
# This code defines the following variables:
#
#  CUDD_FOUND                 - TRUE if CUDD was found.
#  CUDD_INCLUDE_DIRS          - Full paths to all include dirs.
#  CUDD_LIBRARIES             - Full paths to all libraries.
#
# Usage:
#  find_package(CUDD)
#
# The location of CUDD can be specified using the environment variable
# or cmake parameter DOWNWARD_CUDD_ROOT.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

find_path(CUDD_INCLUDE_DIRS
    NAMES cudd.h
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES include
    NO_DEFAULT_PATH
)

find_library(CUDD_OBJ_LIBRARY
    NAMES obj
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES obj 
    NO_DEFAULT_PATH
)

find_library(CUDD_CUDD_LIBRARY
    NAMES cudd
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES cudd 
    NO_DEFAULT_PATH
)

find_library(CUDD_MTR_LIBRARY
    NAMES mtr
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES mtr 
    NO_DEFAULT_PATH
)

find_library(CUDD_ST_LIBRARY
    NAMES st
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES st 
    NO_DEFAULT_PATH
)

find_library(CUDD_UTIL_LIBRARY
    NAMES util
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES util 
    NO_DEFAULT_PATH
)

find_library(CUDD_EPD_LIBRARY
    NAMES epd
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES epd
    NO_DEFAULT_PATH
)

find_library(CUDD_DDDMP_LIBRARY
    NAMES dddmp
    HINTS $ENV{DOWNWARD_CUDD_ROOT}
    PATH_SUFFIXES dddmp 
    NO_DEFAULT_PATH
)

set(CUDD_LIBRARIES ${CUDD_OBJ_LIBRARY} ${CUDD_CUDD_LIBRARY} ${CUDD_MTR_LIBRARY} ${CUDD_ST_LIBRARY} ${CUDD_UTIL_LIBRARY} ${CUDD_EPD_LIBRARY} ${CUDD_DDDMP_LIBRARY})

# Check if everything was found and set CUDD_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    CUDD
    REQUIRED_VARS CUDD_INCLUDE_DIRS CUDD_LIBRARIES CUDD_OBJ_LIBRARY CUDD_CUDD_LIBRARY CUDD_MTR_LIBRARY CUDD_ST_LIBRARY CUDD_UTIL_LIBRARY CUDD_EPD_LIBRARY CUDD_DDDMP_LIBRARY
)

mark_as_advanced(CUDD_INCLUDE_DIRS CUDD_LIBRARIES CUDD_OBJ_LIBRARY CUDD_CUDD_LIBRARY CUDD_MTR_LIBRARY CUDD_ST_LIBRARY CUDD_UTIL_LIBRARY CUDD_EPD_LIBRARY CUDD_DDDMP_LIBRARY)
