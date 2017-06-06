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

#include "ngspice/config.h"
#include "cuda_runtime_api.h"
#include "isrcdefs.h"
#include "ngspice/CUSPICE/CUSPICE.h"

int
cuISRCdestroy
(
GENmodel *inModel
)
{
    ISRCmodel *model = (ISRCmodel *)inModel ;
    ISRCinstance *here ;
    int i ;

    for ( ; model != NULL ; model = model->ISRCnextModel)
    {
        /* Special case VSRCparamGPU.VSRCcoeffsArray */
        i = 0 ;

        for (here = model->ISRCinstances ; here != NULL ; here = here->ISRCnextInstance)
        {
            cudaFree (model->ISRCparamCPU.ISRCcoeffsArray[i]) ;

            i++ ;
        }
        free (model->ISRCparamCPU.ISRCcoeffsArray) ;
        cudaFree (model->ISRCparamGPU.d_ISRCcoeffsArray) ;

        i = 0 ;

        for (here = model->ISRCinstances ; here != NULL ; here = here->ISRCnextInstance)
        {
            free (model->ISRCparamCPU.ISRCcoeffsArrayHost [i]) ;

            i++ ;
        }
        free (model->ISRCparamCPU.ISRCcoeffsArrayHost) ;
        /* ----------------------------------------- */

        /* DOUBLE */
        free (model->ISRCparamCPU.ISRCdcvalueArray) ;
        cudaFree (model->ISRCparamGPU.d_ISRCdcvalueArray) ;

        free (model->ISRCparamCPU.ISRCValueArray) ;
        cudaFree (model->ISRCparamGPU.d_ISRCValueArray) ;

        /* INT */
        free (model->ISRCparamCPU.ISRCdcGivenArray) ;
        cudaFree (model->ISRCparamGPU.d_ISRCdcGivenArray) ;

        free (model->ISRCparamCPU.ISRCfunctionTypeArray) ;
        cudaFree (model->ISRCparamGPU.d_ISRCfunctionTypeArray) ;

        free (model->ISRCparamCPU.ISRCfunctionOrderArray) ;
        cudaFree (model->ISRCparamGPU.d_ISRCfunctionOrderArray) ;
    }

    return (OK) ;
}
