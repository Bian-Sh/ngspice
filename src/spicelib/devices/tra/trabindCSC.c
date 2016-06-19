/**********
Author: 2013 Francesco Lannutti
**********/

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "tradefs.h"
#include "ngspice/sperror.h"
#include "ngspice/klu-binding.h"

#include <stdlib.h>

static
int
BindCompare (const void *a, const void *b)
{
    BindElement *A, *B ;
    A = (BindElement *)a ;
    B = (BindElement *)b ;

    return ((int)(A->Sparse - B->Sparse)) ;
}

int
TRAbindCSC (GENmodel *inModel, CKTcircuit *ckt)
{
    TRAmodel *model = (TRAmodel *)inModel ;
    TRAinstance *here ;
    double *i ;
    BindElement *matched, *BindStruct ;
    size_t nz ;

    BindStruct = ckt->CKTmatrix->CKTbindStruct ;
    nz = (size_t)ckt->CKTmatrix->CKTklunz ;

    /* loop through all the TRA models */
    for ( ; model != NULL ; model = model->TRAnextModel)
    {
        /* loop through all the instances of the model */
        for (here = model->TRAinstances ; here != NULL ; here = here->TRAnextInstance)
        {
            if ((here-> TRAbrEq1 != 0) && (here-> TRAbrEq2 != 0))
            {
                i = here->TRAibr1Ibr2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr1Ibr2Binding = matched ;
                here->TRAibr1Ibr2Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq1 != 0) && (here-> TRAintNode1 != 0))
            {
                i = here->TRAibr1Int1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr1Int1Binding = matched ;
                here->TRAibr1Int1Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq1 != 0) && (here-> TRAnegNode1 != 0))
            {
                i = here->TRAibr1Neg1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr1Neg1Binding = matched ;
                here->TRAibr1Neg1Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq1 != 0) && (here-> TRAnegNode2 != 0))
            {
                i = here->TRAibr1Neg2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr1Neg2Binding = matched ;
                here->TRAibr1Neg2Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq1 != 0) && (here-> TRAposNode2 != 0))
            {
                i = here->TRAibr1Pos2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr1Pos2Binding = matched ;
                here->TRAibr1Pos2Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq2 != 0) && (here-> TRAbrEq1 != 0))
            {
                i = here->TRAibr2Ibr1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr2Ibr1Binding = matched ;
                here->TRAibr2Ibr1Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq2 != 0) && (here-> TRAintNode2 != 0))
            {
                i = here->TRAibr2Int2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr2Int2Binding = matched ;
                here->TRAibr2Int2Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq2 != 0) && (here-> TRAnegNode1 != 0))
            {
                i = here->TRAibr2Neg1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr2Neg1Binding = matched ;
                here->TRAibr2Neg1Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq2 != 0) && (here-> TRAnegNode2 != 0))
            {
                i = here->TRAibr2Neg2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr2Neg2Binding = matched ;
                here->TRAibr2Neg2Ptr = matched->CSC ;
            }

            if ((here-> TRAbrEq2 != 0) && (here-> TRAposNode1 != 0))
            {
                i = here->TRAibr2Pos1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAibr2Pos1Binding = matched ;
                here->TRAibr2Pos1Ptr = matched->CSC ;
            }

            if ((here-> TRAintNode1 != 0) && (here-> TRAbrEq1 != 0))
            {
                i = here->TRAint1Ibr1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAint1Ibr1Binding = matched ;
                here->TRAint1Ibr1Ptr = matched->CSC ;
            }

            if ((here-> TRAintNode1 != 0) && (here-> TRAintNode1 != 0))
            {
                i = here->TRAint1Int1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAint1Int1Binding = matched ;
                here->TRAint1Int1Ptr = matched->CSC ;
            }

            if ((here-> TRAintNode1 != 0) && (here-> TRAposNode1 != 0))
            {
                i = here->TRAint1Pos1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAint1Pos1Binding = matched ;
                here->TRAint1Pos1Ptr = matched->CSC ;
            }

            if ((here-> TRAintNode2 != 0) && (here-> TRAbrEq2 != 0))
            {
                i = here->TRAint2Ibr2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAint2Ibr2Binding = matched ;
                here->TRAint2Ibr2Ptr = matched->CSC ;
            }

            if ((here-> TRAintNode2 != 0) && (here-> TRAintNode2 != 0))
            {
                i = here->TRAint2Int2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAint2Int2Binding = matched ;
                here->TRAint2Int2Ptr = matched->CSC ;
            }

            if ((here-> TRAintNode2 != 0) && (here-> TRAposNode2 != 0))
            {
                i = here->TRAint2Pos2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAint2Pos2Binding = matched ;
                here->TRAint2Pos2Ptr = matched->CSC ;
            }

            if ((here-> TRAnegNode1 != 0) && (here-> TRAbrEq1 != 0))
            {
                i = here->TRAneg1Ibr1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAneg1Ibr1Binding = matched ;
                here->TRAneg1Ibr1Ptr = matched->CSC ;
            }

            if ((here-> TRAnegNode2 != 0) && (here-> TRAbrEq2 != 0))
            {
                i = here->TRAneg2Ibr2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRAneg2Ibr2Binding = matched ;
                here->TRAneg2Ibr2Ptr = matched->CSC ;
            }

            if ((here-> TRAposNode1 != 0) && (here-> TRAintNode1 != 0))
            {
                i = here->TRApos1Int1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRApos1Int1Binding = matched ;
                here->TRApos1Int1Ptr = matched->CSC ;
            }

            if ((here-> TRAposNode1 != 0) && (here-> TRAposNode1 != 0))
            {
                i = here->TRApos1Pos1Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRApos1Pos1Binding = matched ;
                here->TRApos1Pos1Ptr = matched->CSC ;
            }

            if ((here-> TRAposNode2 != 0) && (here-> TRAintNode2 != 0))
            {
                i = here->TRApos2Int2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRApos2Int2Binding = matched ;
                here->TRApos2Int2Ptr = matched->CSC ;
            }

            if ((here-> TRAposNode2 != 0) && (here-> TRAposNode2 != 0))
            {
                i = here->TRApos2Pos2Ptr ;
                matched = (BindElement *) bsearch (&i, BindStruct, nz, sizeof(BindElement), BindCompare) ;
                here->TRApos2Pos2Binding = matched ;
                here->TRApos2Pos2Ptr = matched->CSC ;
            }

        }
    }

    return (OK) ;
}

int
TRAbindCSCComplex (GENmodel *inModel, CKTcircuit *ckt)
{
    TRAmodel *model = (TRAmodel *)inModel ;
    TRAinstance *here ;

    NG_IGNORE (ckt) ;

    /* loop through all the TRA models */
    for ( ; model != NULL ; model = model->TRAnextModel)
    {
        /* loop through all the instances of the model */
        for (here = model->TRAinstances ; here != NULL ; here = here->TRAnextInstance)
        {
            if ((here-> TRAbrEq1 != 0) && (here-> TRAbrEq2 != 0))
                here->TRAibr1Ibr2Ptr = here->TRAibr1Ibr2Binding->CSC_Complex ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAintNode1 != 0))
                here->TRAibr1Int1Ptr = here->TRAibr1Int1Binding->CSC_Complex ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAnegNode1 != 0))
                here->TRAibr1Neg1Ptr = here->TRAibr1Neg1Binding->CSC_Complex ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAnegNode2 != 0))
                here->TRAibr1Neg2Ptr = here->TRAibr1Neg2Binding->CSC_Complex ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAposNode2 != 0))
                here->TRAibr1Pos2Ptr = here->TRAibr1Pos2Binding->CSC_Complex ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAbrEq1 != 0))
                here->TRAibr2Ibr1Ptr = here->TRAibr2Ibr1Binding->CSC_Complex ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAintNode2 != 0))
                here->TRAibr2Int2Ptr = here->TRAibr2Int2Binding->CSC_Complex ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAnegNode1 != 0))
                here->TRAibr2Neg1Ptr = here->TRAibr2Neg1Binding->CSC_Complex ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAnegNode2 != 0))
                here->TRAibr2Neg2Ptr = here->TRAibr2Neg2Binding->CSC_Complex ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAposNode1 != 0))
                here->TRAibr2Pos1Ptr = here->TRAibr2Pos1Binding->CSC_Complex ;

            if ((here-> TRAintNode1 != 0) && (here-> TRAbrEq1 != 0))
                here->TRAint1Ibr1Ptr = here->TRAint1Ibr1Binding->CSC_Complex ;

            if ((here-> TRAintNode1 != 0) && (here-> TRAintNode1 != 0))
                here->TRAint1Int1Ptr = here->TRAint1Int1Binding->CSC_Complex ;

            if ((here-> TRAintNode1 != 0) && (here-> TRAposNode1 != 0))
                here->TRAint1Pos1Ptr = here->TRAint1Pos1Binding->CSC_Complex ;

            if ((here-> TRAintNode2 != 0) && (here-> TRAbrEq2 != 0))
                here->TRAint2Ibr2Ptr = here->TRAint2Ibr2Binding->CSC_Complex ;

            if ((here-> TRAintNode2 != 0) && (here-> TRAintNode2 != 0))
                here->TRAint2Int2Ptr = here->TRAint2Int2Binding->CSC_Complex ;

            if ((here-> TRAintNode2 != 0) && (here-> TRAposNode2 != 0))
                here->TRAint2Pos2Ptr = here->TRAint2Pos2Binding->CSC_Complex ;

            if ((here-> TRAnegNode1 != 0) && (here-> TRAbrEq1 != 0))
                here->TRAneg1Ibr1Ptr = here->TRAneg1Ibr1Binding->CSC_Complex ;

            if ((here-> TRAnegNode2 != 0) && (here-> TRAbrEq2 != 0))
                here->TRAneg2Ibr2Ptr = here->TRAneg2Ibr2Binding->CSC_Complex ;

            if ((here-> TRAposNode1 != 0) && (here-> TRAintNode1 != 0))
                here->TRApos1Int1Ptr = here->TRApos1Int1Binding->CSC_Complex ;

            if ((here-> TRAposNode1 != 0) && (here-> TRAposNode1 != 0))
                here->TRApos1Pos1Ptr = here->TRApos1Pos1Binding->CSC_Complex ;

            if ((here-> TRAposNode2 != 0) && (here-> TRAintNode2 != 0))
                here->TRApos2Int2Ptr = here->TRApos2Int2Binding->CSC_Complex ;

            if ((here-> TRAposNode2 != 0) && (here-> TRAposNode2 != 0))
                here->TRApos2Pos2Ptr = here->TRApos2Pos2Binding->CSC_Complex ;

        }
    }

    return (OK) ;
}

int
TRAbindCSCComplexToReal (GENmodel *inModel, CKTcircuit *ckt)
{
    TRAmodel *model = (TRAmodel *)inModel ;
    TRAinstance *here ;

    NG_IGNORE (ckt) ;

    /* loop through all the TRA models */
    for ( ; model != NULL ; model = model->TRAnextModel)
    {
        /* loop through all the instances of the model */
        for (here = model->TRAinstances ; here != NULL ; here = here->TRAnextInstance)
        {
            if ((here-> TRAbrEq1 != 0) && (here-> TRAbrEq2 != 0))
                here->TRAibr1Ibr2Ptr = here->TRAibr1Ibr2Binding->CSC ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAintNode1 != 0))
                here->TRAibr1Int1Ptr = here->TRAibr1Int1Binding->CSC ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAnegNode1 != 0))
                here->TRAibr1Neg1Ptr = here->TRAibr1Neg1Binding->CSC ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAnegNode2 != 0))
                here->TRAibr1Neg2Ptr = here->TRAibr1Neg2Binding->CSC ;

            if ((here-> TRAbrEq1 != 0) && (here-> TRAposNode2 != 0))
                here->TRAibr1Pos2Ptr = here->TRAibr1Pos2Binding->CSC ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAbrEq1 != 0))
                here->TRAibr2Ibr1Ptr = here->TRAibr2Ibr1Binding->CSC ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAintNode2 != 0))
                here->TRAibr2Int2Ptr = here->TRAibr2Int2Binding->CSC ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAnegNode1 != 0))
                here->TRAibr2Neg1Ptr = here->TRAibr2Neg1Binding->CSC ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAnegNode2 != 0))
                here->TRAibr2Neg2Ptr = here->TRAibr2Neg2Binding->CSC ;

            if ((here-> TRAbrEq2 != 0) && (here-> TRAposNode1 != 0))
                here->TRAibr2Pos1Ptr = here->TRAibr2Pos1Binding->CSC ;

            if ((here-> TRAintNode1 != 0) && (here-> TRAbrEq1 != 0))
                here->TRAint1Ibr1Ptr = here->TRAint1Ibr1Binding->CSC ;

            if ((here-> TRAintNode1 != 0) && (here-> TRAintNode1 != 0))
                here->TRAint1Int1Ptr = here->TRAint1Int1Binding->CSC ;

            if ((here-> TRAintNode1 != 0) && (here-> TRAposNode1 != 0))
                here->TRAint1Pos1Ptr = here->TRAint1Pos1Binding->CSC ;

            if ((here-> TRAintNode2 != 0) && (here-> TRAbrEq2 != 0))
                here->TRAint2Ibr2Ptr = here->TRAint2Ibr2Binding->CSC ;

            if ((here-> TRAintNode2 != 0) && (here-> TRAintNode2 != 0))
                here->TRAint2Int2Ptr = here->TRAint2Int2Binding->CSC ;

            if ((here-> TRAintNode2 != 0) && (here-> TRAposNode2 != 0))
                here->TRAint2Pos2Ptr = here->TRAint2Pos2Binding->CSC ;

            if ((here-> TRAnegNode1 != 0) && (here-> TRAbrEq1 != 0))
                here->TRAneg1Ibr1Ptr = here->TRAneg1Ibr1Binding->CSC ;

            if ((here-> TRAnegNode2 != 0) && (here-> TRAbrEq2 != 0))
                here->TRAneg2Ibr2Ptr = here->TRAneg2Ibr2Binding->CSC ;

            if ((here-> TRAposNode1 != 0) && (here-> TRAintNode1 != 0))
                here->TRApos1Int1Ptr = here->TRApos1Int1Binding->CSC ;

            if ((here-> TRAposNode1 != 0) && (here-> TRAposNode1 != 0))
                here->TRApos1Pos1Ptr = here->TRApos1Pos1Binding->CSC ;

            if ((here-> TRAposNode2 != 0) && (here-> TRAintNode2 != 0))
                here->TRApos2Int2Ptr = here->TRApos2Int2Binding->CSC ;

            if ((here-> TRAposNode2 != 0) && (here-> TRAposNode2 != 0))
                here->TRApos2Pos2Ptr = here->TRApos2Pos2Binding->CSC ;

        }
    }

    return (OK) ;
}
