/*
 * Copyright (c) 2014, NVIDIA Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation and/or 
 *    other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to 
 *    endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "inddefs.h"
#include "ngspice/sperror.h"

#define TopologyMatrixInsert(Ptr, instance_ID, offset, Value, global_ID) \
    ckt->CKTtopologyMatrixCOOi [global_ID] = (int)(here->Ptr - basePtr) ; \
    ckt->CKTtopologyMatrixCOOj [global_ID] = model->PositionVector [instance_ID] + offset ; \
    ckt->CKTtopologyMatrixCOOx [global_ID] = Value ;

#define TopologyMatrixInsertRHS(offset, instance_ID, offsetRHS, Value, global_ID) \
    ckt->CKTtopologyMatrixCOOiRHS [global_ID] = here->offset ; \
    ckt->CKTtopologyMatrixCOOjRHS [global_ID] = model->PositionVectorRHS [instance_ID] + offsetRHS ; \
    ckt->CKTtopologyMatrixCOOxRHS [global_ID] = Value ;

int
INDtopology (GENmodel *inModel, CKTcircuit *ckt, int *i, int *j)
{
    INDmodel *model = (INDmodel *)inModel ;
    INDinstance *here ;
    int k ;
    double *basePtr ;
    basePtr = ckt->CKTmatrix->CKTkluAx ;

    /*  loop through all the inductor models */
    for ( ; model != NULL ; model = model->INDnextModel)
    {
        k = 0 ;

        /* loop through all the instances of the model */
        for (here = model->INDinstances ; here != NULL ; here = here->INDnextInstance)
        {
            if ((here->INDposNode != 0) && (here->INDbrEq != 0))
            {
                TopologyMatrixInsert (INDposIbrPtr, k, 0, 1, *i) ;
                (*i)++ ;
            }

            if ((here->INDnegNode != 0) && (here->INDbrEq != 0))
            {
                TopologyMatrixInsert (INDnegIbrPtr, k, 0, -1, *i) ;
                (*i)++ ;
            }

            if ((here->INDbrEq != 0) && (here->INDnegNode != 0))
            {
                TopologyMatrixInsert (INDibrNegPtr, k, 0, -1, *i) ;
                (*i)++ ;
            }

            if ((here->INDbrEq != 0) && (here->INDposNode != 0))
            {
                TopologyMatrixInsert (INDibrPosPtr, k, 0, 1, *i) ;
                (*i)++ ;
            }

            if ((here->INDbrEq != 0) && (here->INDbrEq != 0))
            {
                TopologyMatrixInsert (INDibrIbrPtr, k, 1, -1, *i) ;
                (*i)++ ;
            }

            if (here->INDbrEq != 0)
            {
                TopologyMatrixInsertRHS (INDbrEq, k, 0, 1, *j) ;
                (*j)++ ;
            }

            k++ ;
        }
    }

    return (OK) ;
}
