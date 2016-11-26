/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
**********/

        /* load the inductor structure with those pointers needed later 
         * for fast matrix loading 
         */

#include "ngspice/ngspice.h"
#include "ngspice/ifsim.h"
#include "ngspice/smpdefs.h"
#include "ngspice/cktdefs.h"
#include "inddefs.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"


#ifdef MUTUAL
/*ARGSUSED*/
int
MUTsetup(SMPmatrix *matrix, GENmodel *inModel, CKTcircuit *ckt, int *states)
{
    MUTmodel *model = (MUTmodel*)inModel;
    MUTinstance *here;
    INDmatrixSet *temp ;
    int ktype;

    NG_IGNORE(states);

    /*  loop through all the inductor models */
    for( ; model != NULL; model = model->MUTnextModel ) {

        /* loop through all the instances of the model */
        for (here = model->MUTinstances; here != NULL ;
                here=here->MUTnextInstance) {
            
            ktype = CKTtypelook("Inductor");
            if(ktype <= 0) {
                SPfrontEnd->IFerrorf (ERR_PANIC,
                        "mutual inductor, but inductors not available!");
                return(E_INTERN);
            }

            if (!here->MUTind1)
                here->MUTind1 = (INDinstance *) CKTfndDev(ckt, here->MUTindName1);
            if (!here->MUTind1) {
                SPfrontEnd->IFerrorf (ERR_WARNING,
                    "%s: coupling to non-existant inductor %s.",
                    here->MUTname, here->MUTindName1);
            }
            if (!here->MUTind2)
                here->MUTind2 = (INDinstance *) CKTfndDev(ckt, here->MUTindName2);
            if (!here->MUTind2) {
                SPfrontEnd->IFerrorf (ERR_WARNING,
                    "%s: coupling to non-existant inductor %s.",
                    here->MUTname, here->MUTindName2);
            }

            /* Assign 'setIndex' and 'matrixIndex' for L matrix */
            if (!here->MUTind1->setPtr && !here->MUTind2->setPtr) {
                /* Create the set */
                here->MUTind1->INDmatrixIndex = 0 ;
                here->MUTind2->INDmatrixIndex = 1 ;

                temp = TMALLOC (INDmatrixSet, 1) ;
                temp->INDmatrixSize = 2 ;
                temp->next = ckt->inductanceMatrixSets ;
                temp->Xindhead = here->MUTind1;
                here->MUTind1->Xnext = here->MUTind2;
                temp->Xmuthead = here;
                ckt->inductanceMatrixSets = temp ;

                here->MUTind1->setPtr = temp ;
                here->MUTind2->setPtr = temp ;
            } else if (here->MUTind1->setPtr && !here->MUTind2->setPtr) {
                /* Add the new MUTind2 into the set */
                temp = here->MUTind1->setPtr ;
                here->MUTind2->INDmatrixIndex = temp->INDmatrixSize ;
                temp->INDmatrixSize++ ;
                here->MUTind2->Xnext = temp->Xindhead;
                temp->Xindhead = here->MUTind2;
                here->Xnext = temp->Xmuthead;
                temp->Xmuthead = here;

                here->MUTind2->setPtr = temp ;
            } else if (!here->MUTind1->setPtr && here->MUTind2->setPtr) {
                /* Add the new MUTind1 into the set */
                temp = here->MUTind2->setPtr ;
                here->MUTind1->INDmatrixIndex = temp->INDmatrixSize ;
                temp->INDmatrixSize++ ;
                here->MUTind1->Xnext = temp->Xindhead;
                temp->Xindhead = here->MUTind1;
                here->Xnext = temp->Xmuthead;
                temp->Xmuthead = here;

                here->MUTind1->setPtr = temp ;
            } else if (here->MUTind1->setPtr == here->MUTind2->setPtr) {
                /* Add only the K coefficient into the set */
                temp = here->MUTind2->setPtr ;
                here->Xnext = temp->Xmuthead;
                temp->Xmuthead = here;
            } else {
                fprintf(stderr, "Ouch, FIXME, this case is not yet coded\n");
            }

/* macro to make elements with built in test for out of memory */
#define TSTALLOC(ptr,first,second) \
do { if((here->ptr = SMPmakeElt(matrix, here->first, here->second)) == NULL){\
    return(E_NOMEM);\
} } while(0)

            TSTALLOC(MUTbr1br2,MUTind1->INDbrEq,MUTind2->INDbrEq);
            TSTALLOC(MUTbr2br1,MUTind2->INDbrEq,MUTind1->INDbrEq);
        }

        /* Allocate the correct space for the L matrix of each set */
        temp = ckt->inductanceMatrixSets ;
        while (temp != NULL) {
           temp->INDmatrix = TMALLOC (double, temp->INDmatrixSize * temp->INDmatrixSize) ;
           temp = temp->next ;
        }
    }
    return(OK);
}
#endif /* MUTUAL */
