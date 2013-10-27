/* CKTsoaCheck(ckt)
 *
 * this is a driver program to iterate through all the various SOA check
 * functions provided for the circuit elements in the given circuit */

#include "ngspice/config.h"
#include "ngspice/devdefs.h"
#include "ngspice/sperror.h"
#include "ngspice/ifsim.h"
#include "ngspice/devdefs.h"

#include "dev.h"


int
CKTsoaCheck(CKTcircuit *ckt)
{
    int i;
    int error;
    SPICEdev **devs = devices();

    for (i = 0; i < DEVmaxnum; i++)
        if (devs[i] && devs[i]->DEVsoaCheck && ckt->CKThead[i]) {
            error = devs[i]->DEVsoaCheck (ckt, ckt->CKThead[i]);
            if (error)
                return error;
        }

    return OK;
}
