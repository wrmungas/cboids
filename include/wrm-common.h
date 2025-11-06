#ifndef WRM_COMMON_H
#define WRM_COMMON_H

/*
File wrm-common.h

Created  Oct 29, 2025 
by William R Mungas (wrm)

Version: 0.1.0 
(Last modified Nov 3, 2025)

DESCRIPTION:
Standard baseline for my wrm utility headers, used by all of the others.
These utilities are intended as a set to be included from a single 'wrm' 
folder, and all will use this one for common definitions, C standard includes,
and other generally useful functions and types.

Anything more niche and unnecessary/not used by the other utilities will be
found in wrm-misc instead.

PROVIDES:
- several C standard library #includes to establish baseline functionality (or wrap)
- bit-sized integer types: u8-u64 and i8-i64
- option, pair, and handle types
- a printf-like fail() function to exit with a given code and a stderr message
- a string matching function to match a string against a set and return an integer,
    for usage with switch statements; and a macro for slightly cleaner syntax

REQUIREMENTS:
The current version of this file is header-only: YOU, the user, must have the 
following in some .c file in your project:
    #define WRM_COMMON_IMPLEMENTATION
    #include "wrm-common.h"

This is necessary to define the functions declared in this file properly
*/

/* standard set of includes */
#include <stdlib.h> // malloc and free
#include <stdio.h> // print and file io functions
#include <stdbool.h> // booleans! thank fuck
#include <stddef.h> // size_t, NULL! thank fuck
#include <stdint.h> // int<n>_t, uint<n>_t
#include <string.h> // string comparison, mostly so I can wrap it myself
#include <stdarg.h> // so I can do unnecessarily complicated things with varargs :P
#include <math.h> // so I can do fucking math

/*
Sized-integer typedefs: these are the preferred names across all of my
utilities
*/

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;


/* 
For when I don't really care about the exact integer size, but I do care about 
signedness - simply much shorter to type
*/

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

// internal marker: clearer for modules
#define internal static


// pair and option types 

/*
Pair type
Must specify:
- the full type names (t1, t2)
- the variable-friendly names (t1_name, t2_name)
ex DEFINE_PAIR(int, int, const char*, cstr): wrm_Pair_int_cstr { int, const char *};
*/
#define DEFINE_PAIR(t1, t1_name, t2, t2_name) typedef struct wrm_Pair_ ## t1_name ## _ ## t2_name { \
    t1 t1_name ## _val; \
    t2 t2_name ## _val; \
} wrm_Pair ## t1_name ## _ ## t2_name

/*
Option type
Must specify:
- the full type name (t)
- the variable-friendly name (t_name)
ex DEFINE_OPTION(const char *, cstr): wrm_Option_cstr { const char * };
*/
#define DEFINE_OPTION(t, t_name) typedef struct wrm_Option_ ## t_name { \
    bool exists; \
    t t_name ## _val; \
} wrm_Option_ ## t_name

#define OPTION_NONE(t_name) (wrm_Option_ ## t_name){.exists = false}

// handle type
typedef u32 wrm_Handle;

DEFINE_OPTION(wrm_Handle, Handle);

// list struct type
#define DEFINE_LIST(t, t_name) typedef struct wrm_List_ ## t_name { \
    u32 cap; \
    u32 len; \
    t *data; \
} wrm_List_ ## t_name

// define some utility functions (might extract to specific 'wrm-..' files)

/* 
For critical program failure
Exits with the given code and error message
Works like printf
*/
void wrm_fail(u8 code, const char *format, ...)
__attribute__((format(printf, 2, 3))); // for gcc printf treatment

/*
Compares two c-style strings
Length-bounded by n
*/
bool wrm_cstrn_eq(size_t n, const char *str1, const char *str2);

/*
Matches the src string with the first of the options
Returns 1 if the first option is matched, 2 if the second...
Returns zero if no options are matched
Can use this for switch statements on a set of strings
*/
int wrm_cstr_match(const char *src, const char **opts, size_t n_opts);

/*
Matches the src string with the first of the options
Comparisons are limited to the first n characters
Returns 1 if the first option is matched, 2 if the second...
Returns zero if no options are matched
Can use this for switch statements on a set of strings
*/
int wrm_cstrn_match(size_t len, const char *src, const char **opts, size_t n_opts);

/*
Possibly dangerous match syntax macro for several c-style strings
Returns 1 if the first option is matched, 2 if the second...
Returns zero if no options are matched
Can use this for switch statements on a set of strings
*/
#define wrm_MATCH(C, ...) wrm_str_match(C, \
    (const char*[]){__VA_ARGS__}, \
    sizeof (const char*[]){__VA_ARGS__} / sizeof (const char*) )

// header-only utility file: must define WRM_COMMON_IMPLEMENTATION somewhere in project

#ifdef WRM_COMMON_IMPLEMENTATION // begin header implementation

void wrm_fail(u8 code, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(code);
}

bool wrm_cstrn_eq(size_t n, const char *str1, const char *str2) 
{
    return strncmp(str1, str2, n) == 0;
}

int wrm_cstr_match(const char *src, const char **opts, size_t n_opts)
{
    for(u8 i = 0; i < n_opts; i++) {
        if(strcmp(src, opts[i]) == 0) {
            return i + 1;
        }
    }
    return 0;
}

int wrm_cstrn_match(size_t len, const char *src, const char **opts, size_t n_opts)
{
    for(u8 i = 0; i < n_opts; i++) {
        if(wrm_cstrn_eq(len, src, opts[i])) {
            return i + 1;
        }
    }
    return 0;
}

#endif // end header implementation

#endif // end include guards