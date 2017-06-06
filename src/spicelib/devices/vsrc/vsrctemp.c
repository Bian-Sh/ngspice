/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
**********/

#include "ngspice/ngspice.h"
#include "ngspice/smpdefs.h"
#include "ngspice/cktdefs.h"
#include "vsrcdefs.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"

#ifdef USE_CUSPICE
#include "ngspice/CUSPICE/CUSPICE.h"
#endif

/*ARGSUSED*/
int
VSRCtemp(GENmodel *inModel, CKTcircuit *ckt)
        /* Pre-process voltage source parameters
         */
{
    VSRCmodel *model = (VSRCmodel *)inModel;
    VSRCinstance *here;
    double radians;

    NG_IGNORE(ckt);

#ifdef USE_CUSPICE
    int i, j, status ;
#endif

    /*  loop through all the voltage source models */
    for( ; model != NULL; model = model->VSRCnextModel ) {

#ifdef USE_CUSPICE
    i = 0 ;
#endif

        /* loop through all the instances of the model */
        for (here = model->VSRCinstances; here != NULL ;
                here=here->VSRCnextInstance) {

            if(here->VSRCacGiven && !here->VSRCacMGiven) {
                here->VSRCacMag = 1;
            }
            if(here->VSRCacGiven && !here->VSRCacPGiven) {
                here->VSRCacPhase = 0;
            }
            if(!here->VSRCdcGiven) {
                /* no DC value - either have a transient value, or none */
                if(here->VSRCfuncTGiven) {
                    SPfrontEnd->IFerrorf (ERR_WARNING,
                            "%s: no DC value, transient time 0 value used",
                            here->VSRCname);
                } else {
                    SPfrontEnd->IFerrorf (ERR_WARNING,
                            "%s: has no value, DC 0 assumed",
                            here->VSRCname);
                }
            }
            radians = here->VSRCacPhase * M_PI / 180.0;
            here->VSRCacReal = here->VSRCacMag * cos(radians);
            here->VSRCacImag = here->VSRCacMag * sin(radians);

#ifdef USE_CUSPICE
            for (j = 0 ; j < here->n_coeffs ; j++)
            {
                model->VSRCparamCPU.VSRCcoeffsArrayHost [i] [j] = here->VSRCcoeffs [j] ;
            }

            model->VSRCparamCPU.VSRCdcvalueArray[i] = here->VSRCdcValue ;
            model->VSRCparamCPU.VSRCrdelayArray[i] = here->VSRCrdelay ;
            model->VSRCparamCPU.VSRCdcGivenArray[i] = here->VSRCdcGiven ;
            model->VSRCparamCPU.VSRCfunctionTypeArray[i] = here->VSRCfunctionType ;
            model->VSRCparamCPU.VSRCfunctionOrderArray[i] = here->VSRCfunctionOrder ;
            model->VSRCparamCPU.VSRCrGivenArray[i] = here->VSRCrGiven ;
            model->VSRCparamCPU.VSRCrBreakptArray[i] = here->VSRCrBreakpt ;

            i++ ;
#endif

        }

#ifdef USE_CUSPICE
        status = cuVSRCtemp ((GENmodel *)model) ;
        if (status != 0)
            return (E_NOMEM) ;
#endif

    }

    return (OK) ;
}
