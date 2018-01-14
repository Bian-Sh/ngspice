/**********
Copyright 1991 Regents of the University of California.  All rights reserved.
Author: 1987 Kartikeya Mayaram, U. C. Berkeley CAD Group
**********/

/*
 * This routine deletes a NUMOS model from the circuit and frees the storage
 * it was using. returns an error if the model has instances
 */

#include "ngspice/ngspice.h"
#include "numosdef.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"


int
NUMOSmDelete(GENmodel *model)
{
    GENmodelFree(model);
    return OK;
}
