/**********
Copyright 2003 Paolo Nenzi
Author: 2003 Paolo Nenzi
**********/
/*
 */

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "inddefs.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"

#ifdef USE_CUSPICE
#include "ngspice/CUSPICE/CUSPICE.h"
#endif

/*ARGSUSED*/
int
INDtemp(GENmodel *inModel, CKTcircuit *ckt)
{
    INDmodel *model = (INDmodel*)inModel;
    INDinstance *here;
    double difference;
    double factor;
    double tc1, tc2;

#ifdef USE_CUSPICE
    int i, status ;
#endif

    /*  loop through all the inductor models */
    for( ; model != NULL; model = model->INDnextModel ) {

#ifdef USE_CUSPICE
    i = 0 ;
#endif

        /* loop through all the instances of the model */
        for (here = model->INDinstances; here != NULL ;
                here=here->INDnextInstance) {

            /* Default Value Processing for Inductor Instance */

            if(!here->INDtempGiven) {
                here->INDtemp = ckt->CKTtemp;
                if(!here->INDdtempGiven)   here->INDdtemp  = 0.0;
            } else { /* INDtempGiven */
                here->INDdtemp = 0.0;
                if (here->INDdtempGiven)
                    printf("%s: Instance temperature specified, dtemp ignored\n",
                           here->INDname);
            }

            if (!here->INDscaleGiven) here->INDscale = 1.0;
            if (!here->INDmGiven)     here->INDm     = 1.0;
            if (!here->INDntGiven)    here->INDnt    = 0.0;

            if (!here->INDindGiven) { /* No instance inductance given */
                if (here->INDntGiven)
                    here->INDinduct = model->INDspecInd * here->INDnt * here->INDnt;
                else
                    here->INDinduct = model->INDmInd;
            }
            difference = (here->INDtemp + here->INDdtemp) - model->INDtnom;

            /* instance parameters tc1 and tc2 will override
               model parameters tc1 and tc2 */
            if (here->INDtc1Given)
                tc1 = here->INDtc1; /* instance */
            else
                tc1 = model->INDtempCoeff1; /* model */

            if (here->INDtc2Given)
                tc2 = here->INDtc2;
            else
                tc2 = model->INDtempCoeff2;

            factor = 1.0 + tc1*difference + tc2*difference*difference;

            here->INDinduct = here->INDinduct * factor * here->INDscale;

#ifdef USE_CUSPICE
            model->INDparamCPU.INDinitCondArray[i] = here->INDinitCond ;
            model->INDparamCPU.INDinductArray[i] = here->INDinduct ;
            model->INDparamCPU.INDbrEqArray[i] = here->INDbrEq ;
            model->INDparamCPU.INDstateArray[i] = here->INDstate ;

            i++ ;
#endif

        }

#ifdef USE_CUSPICE
        status = cuINDtemp ((GENmodel *)model) ;
        if (status != 0)
            return (E_NOMEM) ;
#endif

    }
    return (OK) ;
}

