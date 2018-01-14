/*============================================================================
FILE    MIFmDelete.c

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

    This file contains the function called by SPICE to delete a model
    structure and all instances of that model.

INTERFACES

    MIFmDelete()

REFERENCED FILES

    None.

NON-STANDARD FEATURES

    None.

============================================================================*/

/* #include "prefix.h"  */
#include "ngspice/ngspice.h"
#include <stdio.h>
//#include "util.h"
#include "ngspice/sperror.h"
#include "ngspice/gendefs.h"

#include "ngspice/mifproto.h"
#include "ngspice/mifdefs.h"

/* #include "suffix.h" */


/*
MIFmDelete

This function deletes a particular model defined by a .model card
from the linked list of model structures of a particular code
model type, freeing all dynamically allocated memory used by the
model structure.  It calls MIFdelete as needed to delete all
instances of the specified model.
*/

int MIFmDelete(GENmodel *model)
{
    MIFmodel *here = (MIFmodel*) model;
    int       i;

    /* Free the model params stuff allocated in MIFget_mod */
    for (i = 0; i < here->num_param; i++) {
        if (here->param[i]->element)
            FREE(here->param[i]->element);
        FREE(here->param[i]);
    }
    FREE(here->param);

    /* Free the model and return */
    GENmodelFree(GENmodelOf(here));
    return(OK);
}
