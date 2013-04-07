/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
**********/

#include "ngspice/ngspice.h"
#include <stdio.h>
#include "ngspice/ifsim.h"
#include "ngspice/cktdefs.h"
#include "ngspice/const.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"

/* This is a routine to initialize the sensitivity
 * data structure
 */

int
SENstartup(CKTcircuit *ckt, int restart)
{
    int i;
    int err;
    IFvalue parmtemp;
    int type;
    GENinstance *fast;

    if (restart) {
        fprintf(stderr, "unsupported code, (SENstartup), doesn't support restart\n");
        exit(1);
    }

#ifdef SENSDEBUG
    printf("SENstartup\n");
#endif

    ckt->CKTsenInfo->SENstatus = NORMAL;
    ckt->CKTsenInfo->SENpertfac = 1e-4;
    ckt->CKTsenInfo->SENinitflag = ON; /* allocate memory in NIsenReinit */

    parmtemp.iValue = 1;

    for (i = 0; i < ckt->CKTsenInfo->SENnumVal; i++) {
        type = -1;
        fast = NULL;

        err = CKTfndDev(ckt, &type, &fast,
                        ckt->CKTsenInfo->SENdevices[i],
                        NULL, NULL);
        if (err != OK)
            return err;

        err = CKTpName(
            ckt->CKTsenInfo->SENparmNames[i],
            &parmtemp, ckt, type,
            ckt->CKTsenInfo->SENdevices[i],
            &fast);
        if (err != OK)
            return err;
    }

#ifdef SENSDEBUG
    printf("SENstartup end\n");
#endif

    return OK;
}
