/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2014, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "src/GitSHA1.h"
const char* get_version_sha1()
{
    static const char myversion_sha1[] = "8a9089f1d7207db73381d331666263e389c9a496";
    return myversion_sha1;
}

const char* get_version_tag()
{
    static const char myversion_tag[] = "5.0.1";
    return myversion_tag;
}

const char* get_compilation_env()
{
    static const char compilation_env[] =
    "CMAKE_CXX_COMPILER = /usr/bin/c++ | "
    "CMAKE_CXX_FLAGS =  -std=c++11 -g -pthread -Wall -Wextra -Wunused -pedantic -Wsign-compare -fno-omit-frame-pointer -Wtype-limits -Wuninitialized -Wno-deprecated -Wstrict-aliasing -Wpointer-arith -fvisibility=hidden -Wpointer-arith -Wformat-nonliteral -Winit-self -Wparentheses -Wunreachable-code -ggdb3 -fPIC | "
    "COMPILE_DEFINES =  -DUSE_PTHREADS -DBOOST_TEST_DYN_LINK -DUSE_ZLIB | "
    "STATICCOMPILE = OFF | "
    "ONLY_SIMPLE = OFF | "
    "Boost_FOUND = 1 | "
    "TBB_FOUND =  | "
    "STATS = OFF | "
    "MYSQL_FOUND =  | "
    "SQLITE3_FOUND =  | "
    "ZLIB_FOUND = TRUE | "
    "VALGRIND_FOUND = TRUE | "
    "ENABLE_TESTING = OFF | "
    "M4RI_FOUND = FALSE | "
    "SLOW_DEBUG = OFF | "
    "ENABLE_ASSERTIONS = ON | "
    "PYTHON_EXECUTABLE = /usr/bin/python2.7 | "
    "PYTHON_LIBRARY = /usr/lib/x86_64-linux-gnu/libpython2.7.so | "
    "PYTHON_INCLUDE_DIRS = /usr/include/python2.7 | "
    "MY_TARGETS =  | "
    "compilation date time = " __DATE__ " " __TIME__
    ""
    ;
    return compilation_env;
}
