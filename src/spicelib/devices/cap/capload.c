/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified: September 2003 Paolo Nenzi
**********/

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "capdefs.h"
#include "ngspice/trandefs.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"

int
CAPload(GENmodel *inModel, CKTcircuit *ckt)
/* actually load the current capacitance value into the
 * sparse matrix previously provided
 */
{
    CAPmodel *model = (CAPmodel*)inModel;
    CAPinstance *here;
    int cond1;
    double vcap;
    double geq;
    double ceq;
    int error;
    double m;

    /* check if capacitors are in the circuit or are open circuited */
    if(ckt->CKTmode & (MODETRAN|MODEAC|MODETRANOP) ) {
        /* evaluate device independent analysis conditions */
        cond1=
            ( ( (ckt->CKTmode & MODEDC) &&
                (ckt->CKTmode & MODEINITJCT) )
              || ( ( ckt->CKTmode & MODEUIC) &&
                   ( ckt->CKTmode & MODEINITTRAN) ) ) ;
        /*  loop through all the capacitor models */
        for( ; model != NULL; model = model->CAPnextModel ) {

            /* loop through all the instances of the model */
            for (here = model->CAPinstances; here != NULL ;
                    here=here->CAPnextInstance) {

                m = here->CAPm;

                if(cond1) {
                    vcap = here->CAPinitCond;
                } else if (here->CAPbranch == 0) {
                    vcap = *(ckt->CKTrhsOld+here->CAPposNode) -
                           *(ckt->CKTrhsOld+here->CAPnegNode) ;
                } else {
                    vcap = *(ckt->CKTrhsOld+here->CAPposNode) -
                           *(ckt->CKTrhsOld+here->CAPauxNode) ;
                }
                if(ckt->CKTmode & (MODETRAN | MODEAC)) {
#ifndef PREDICTOR
                    if(ckt->CKTmode & MODEINITPRED) {
                        *(ckt->CKTstate0+here->CAPqcap) =
                            *(ckt->CKTstate1+here->CAPqcap);
                    } else { /* only const caps - no poly's */
#endif /* PREDICTOR */
                        *(ckt->CKTstate0+here->CAPqcap) = here->CAPcapac * vcap;
                        if((ckt->CKTmode & MODEINITTRAN)) {
                            *(ckt->CKTstate1+here->CAPqcap) =
                                *(ckt->CKTstate0+here->CAPqcap);
                        }
#ifndef PREDICTOR
                    }
#endif /* PREDICTOR */
                    error = NIintegrate(ckt,&geq,&ceq,here->CAPcapac,
                                        here->CAPqcap);
                    if(error) return(error);
                    if(ckt->CKTmode & MODEINITTRAN) {
                        *(ckt->CKTstate1+here->CAPccap) =
                            *(ckt->CKTstate0+here->CAPccap);
                    }
                    *(here->CAPposPosptr) += m * geq;
                    *(here->CAPnegNegptr) += m * geq;
                    *(here->CAPposNegptr) -= m * geq;
                    *(here->CAPnegPosptr) -= m * geq;

                    if (here->CAPbranch == 0) {
                        *(ckt->CKTrhs+here->CAPposNode) -= m * ceq;
                        *(ckt->CKTrhs+here->CAPnegNode) += m * ceq;
                    } else {
                        *(here->VSRCposIbrptr) += 1.0 ;
                        *(here->VSRCnegIbrptr) -= 1.0 ;
                        *(here->VSRCibrPosptr) += 1.0 ;
                        *(here->VSRCibrNegptr) -= 1.0 ;
                        *(ckt->CKTrhs+here->CAPposNode) -= m * ceq;
                        *(ckt->CKTrhs+here->CAPauxNode) += m * ceq;
                    }
                } else {
                    if (here->CAPbranch) {
                        *(here->VSRCposIbrptr) += 1.0 ;
                        *(here->VSRCnegIbrptr) -= 1.0 ;
                        *(here->VSRCibrPosptr) += 1.0 ;
                        *(here->VSRCibrNegptr) -= 1.0 ;
                    }
                    *(ckt->CKTstate0+here->CAPqcap) = here->CAPcapac * vcap;
                }
            }
        }
    } else {
        for (; model; model = model->CAPnextModel)
            for (here = model->CAPinstances; here; here=here->CAPnextInstance)
                if (here->CAPbranch) {
                    *(here->VSRCposIbrptr) += 1.0 ;
                    *(here->VSRCnegIbrptr) -= 1.0 ;
                    *(here->VSRCibrPosptr) += 1.0 ;
                    *(here->VSRCibrNegptr) -= 1.0 ;
                }
    }
    return(OK);
}

