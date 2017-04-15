/**********
Copyright 2003 ??.  All rights reserved.
Author: 2003 Paolo Nenzi
**********/

#include "ngspice/ngspice.h"
#include "bsim1def.h"
#include "ngspice/cktdefs.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/noisedef.h"
#include "ngspice/suffix.h"

/*
 * B1noise (mode, operation, firstModel, ckt, data, OnDens)
 *    This routine names and evaluates all of the noise sources
 *    associated with MOSFET's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the MOSFET's is summed with the variable "OnDens".
 */


int
B1noise (int mode, int operation, GENmodel *genmodel, CKTcircuit *ckt, 
           Ndata *data, double *OnDens)
{
    NOISEAN *job = (NOISEAN *) ckt->CKTcurJob;

    B1model *firstModel = (B1model *) genmodel;
    B1model *model;
    B1instance *inst;
    char name[N_MXVLNTH];
    double tempOnoise;
    double tempInoise;
    double noizDens[B1NSRCS];
    double lnNdens[B1NSRCS];
    int i;

    /* define the names of the noise sources */

    static char *B1nNames[B1NSRCS] = {       /* Note that we have to keep the order */
	"_rd",              /* noise due to rd */        /* consistent with thestrchr definitions */
	"_rs",              /* noise due to rs */        /* in bsim1defs.h */
	"_id",              /* noise due to id */
	"_1overf",          /* flicker (1/f) noise */
	""                  /* total transistor noise */
    };

    for (model=firstModel; model != NULL; model=model->B1nextModel) {
	for (inst=model->B1instances; inst != NULL; inst=inst->B1nextInstance) {
        
	    switch (operation) {

	    case N_OPEN:

		/* see if we have to to produce a summary report */
		/* if so, name all the noise generators */

		if (job->NStpsSm != 0) {
		    switch (mode) {

		    case N_DENS:
			for (i=0; i < B1NSRCS; i++) {
			    NOISE_ADD_OUTVAR(ckt, data, "onoise_%s%s", inst->B1name, B1nNames[i]);


			}
			break;

		    case INT_NOIZ:
			for (i=0; i < B1NSRCS; i++) {
			    NOISE_ADD_OUTVAR(ckt, data, "onoise_total_%s%s", inst->B1name, B1nNames[i]);


			    NOISE_ADD_OUTVAR(ckt, data, "inoise_total_%s%s", inst->B1name, B1nNames[i]);



			}
			break;
		    }
		}
		break;

	    case N_CALC:
		switch (mode) {

		case N_DENS:
		    NevalSrc(&noizDens[B1RDNOIZ],&lnNdens[B1RDNOIZ],
				 ckt,THERMNOISE,inst->B1dNodePrime,inst->B1dNode,
				 inst->B1drainConductance * inst->B1m);

		    NevalSrc(&noizDens[B1RSNOIZ],&lnNdens[B1RSNOIZ],
				 ckt,THERMNOISE,inst->B1sNodePrime,inst->B1sNode,
				 inst->B1sourceConductance * inst->B1m);

		    NevalSrc(&noizDens[B1IDNOIZ],&lnNdens[B1IDNOIZ],
				 ckt,THERMNOISE,inst->B1dNodePrime,inst->B1sNodePrime,
                                 (2.0/3.0 * fabs(inst->B1gm * inst->B1m)));

		    NevalSrc(&noizDens[B1FLNOIZ], NULL, ckt,
				 N_GAIN,inst->B1dNodePrime, inst->B1sNodePrime,
				 (double)0.0);
		    noizDens[B1FLNOIZ] *= model->B1fNcoef * inst->B1m *
				 exp(model->B1fNexp *
				 log(MAX(fabs(inst->B1cd),N_MINLOG))) /
				 (data->freq *
				 (inst->B1w - model->B1deltaW * 1e-6) *
				 (inst->B1l - model->B1deltaL * 1e-6) *
				 model->B1Cox * model->B1Cox);
		    lnNdens[B1FLNOIZ] = 
				 log(MAX(noizDens[B1FLNOIZ],N_MINLOG));

		    noizDens[B1TOTNOIZ] = noizDens[B1RDNOIZ] +
						     noizDens[B1RSNOIZ] +
						     noizDens[B1IDNOIZ] +
						     noizDens[B1FLNOIZ];
		    lnNdens[B1TOTNOIZ] = 
				 log(MAX(noizDens[B1TOTNOIZ], N_MINLOG));

		    *OnDens += noizDens[B1TOTNOIZ];

		    if (data->delFreq == 0.0) { 

			/* if we haven't done any previous integration, we need to */
			/* initialize our "history" variables                      */

			for (i=0; i < B1NSRCS; i++) {
			    inst->B1nVar[LNLSTDENS][i] = lnNdens[i];
			}

			/* clear out our integration variables if it's the first pass */

			if (data->freq == job->NstartFreq) {
			    for (i=0; i < B1NSRCS; i++) {
				inst->B1nVar[OUTNOIZ][i] = 0.0;
				inst->B1nVar[INNOIZ][i] = 0.0;
			    }
			}
		    } else {   /* data->delFreq != 0.0 (we have to integrate) */
			for (i=0; i < B1NSRCS; i++) {
			    if (i != B1TOTNOIZ) {
				tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
				      inst->B1nVar[LNLSTDENS][i], data);
				tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
				      lnNdens[i] + data->lnGainInv,
				      inst->B1nVar[LNLSTDENS][i] + data->lnGainInv,
				      data);
				inst->B1nVar[LNLSTDENS][i] = lnNdens[i];
				data->outNoiz += tempOnoise;
				data->inNoise += tempInoise;
				if (job->NStpsSm != 0) {
				    inst->B1nVar[OUTNOIZ][i] += tempOnoise;
				    inst->B1nVar[OUTNOIZ][B1TOTNOIZ] += tempOnoise;
				    inst->B1nVar[INNOIZ][i] += tempInoise;
				    inst->B1nVar[INNOIZ][B1TOTNOIZ] += tempInoise;
                                }
			    }
			}
		    }
		    if (data->prtSummary) {
			for (i=0; i < B1NSRCS; i++) {     /* print a summary report */
			    data->outpVector[data->outNumber++] = noizDens[i];
			}
		    }
		    break;

		case INT_NOIZ:        /* already calculated, just output */
		    if (job->NStpsSm != 0) {
			for (i=0; i < B1NSRCS; i++) {
			    data->outpVector[data->outNumber++] = inst->B1nVar[OUTNOIZ][i];
			    data->outpVector[data->outNumber++] = inst->B1nVar[INNOIZ][i];
			}
		    }    /* if */
		    break;
		}    /* switch (mode) */
		break;

	    case N_CLOSE:
		return (OK);         /* do nothing, the main calling routine will close */
		break;               /* the plots */
	    }    /* switch (operation) */
	}    /* for inst */
    }    /* for model */

return(OK);
}
