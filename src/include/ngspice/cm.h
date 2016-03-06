#ifndef ngspice_CM_H
#define ngspice_CM_H

/* ===========================================================================
FILE    CM.h

MEMBER OF process XSPICE

Copyright 1991
Georgia Tech Research Corporation
Atlanta, Georgia 30332
All Rights Reserved

PROJECT A-8503

AUTHORS

    9/12/91  Bill Kuhn

MODIFICATIONS

    <date> <person name> <nature of modifications>

SUMMARY

    This file is includes all include data in the CM package.

INTERFACES

    None.

REFERENCED FILES

    None.

NON-STANDARD FEATURES

    None.

=========================================================================== */

#include "ngspice/config.h"
#include "ngspice/cmtypes.h"
#include "ngspice/cmconstants.h"
#include "ngspice/cmproto.h"
#include "ngspice/mifcmdat.h"

#if defined (_MSC_VER)
#ifndef NAN
    static const __int64 global_nan = 0x7ff8000000000000i64;
    #define NAN (*(const double *) &global_nan)
#endif
#endif

#if !defined(NAN)
#define NAN (0.0/0.0)
#endif


/*
 * type safe variants of the <ctype.h> functions for char arguments
 */

#if !defined(CHAR_TO_INT)
#define CHAR_TO_INT

inline static int char_to_int(char c) { return (unsigned char) c; }

#define isalpha_c(x) isalpha(char_to_int(x))
#define islower_c(x) islower(char_to_int(x))
#define isdigit_c(x) isdigit(char_to_int(x))
#define isalnum_c(x) isalnum(char_to_int(x))
#define isprint_c(x) isprint(char_to_int(x))
#define isblank_c(x) isblank(char_to_int(x))
#define isspace_c(x) isspace(char_to_int(x))
#define isupper_c(x) isupper(char_to_int(x))

#define tolower_c(x) ((char) tolower(char_to_int(x)))
#define toupper_c(x) ((char) toupper(char_to_int(x)))

#endif


#endif
