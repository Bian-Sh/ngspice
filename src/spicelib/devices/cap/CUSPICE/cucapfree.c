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
#include "capdefs.h"
#include "ngspice/CUSPICE/CUSPICE.h"

int
cuCAPdestroy
(
GENmodel *inModel
)
{
    CAPmodel *model = (CAPmodel *)inModel ;

    for ( ; model != NULL ; model = model->CAPnextModel)
    {
        /* DOUBLE */
        free (model->CAPparamCPU.CAPinitCondArray) ;
        cudaFree (model->CAPparamGPU.d_CAPinitCondArray) ;

        free (model->CAPparamCPU.CAPcapacArray) ;
        cudaFree (model->CAPparamGPU.d_CAPcapacArray) ;

        free (model->CAPparamCPU.CAPmArray) ;
        cudaFree (model->CAPparamGPU.d_CAPmArray) ;

        free (model->CAPparamCPU.CAPgeqValueArray) ;
        cudaFree (model->CAPparamGPU.d_CAPgeqValueArray) ;

        free (model->CAPparamCPU.CAPceqValueArray) ;
        cudaFree (model->CAPparamGPU.d_CAPceqValueArray) ;

        /* INT */
        free (model->CAPparamCPU.CAPposNodeArray) ;
        cudaFree (model->CAPparamGPU.d_CAPposNodeArray) ;

        free (model->CAPparamCPU.CAPnegNodeArray) ;
        cudaFree (model->CAPparamGPU.d_CAPnegNodeArray) ;

        free (model->CAPparamCPU.CAPstateArray) ;
        cudaFree (model->CAPparamGPU.d_CAPstateArray) ;
    }

    return (OK) ;
}
