/**********
 Author: 2010-05 Stefano Perticaroli ``spertica''
 First Review: 2012-02 Francesco Lannutti and Stefano Perticaroli ``spertica''
 Second Review: 2012-10 Stefano Perticaroli ``spertica'' and Francesco Lannutti
**********/

/* Include files for the PSS analysis */
#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "ngspice/pssdefs.h"
#include "ngspice/sperror.h"
#include "cktaccept.h"
#include "ngspice/fteext.h"

#ifdef XSPICE
/* gtri - add - wbk - Add headers */
#include "ngspice/miftypes.h"
#include "ngspice/evt.h"
#include "ngspice/mif.h"
#include "ngspice/evtproto.h"
#include "ngspice/ipctiein.h"
/* gtri - end - wbk - Add headers */
#endif

#ifdef CLUSTER
#include "ngspice/cluster.h"
#endif

#ifdef HAS_WINDOWS /* hvogt 10.03.99, nach W. Mues */
void SetAnalyse (char * Analyse, int Percent) ;
#endif


#define INIT_STATS() \
do { \
    startTime = SPfrontEnd->IFseconds();        \
    startIters = ckt->CKTstat->STATnumIter;     \
    startdTime = ckt->CKTstat->STATdecompTime;  \
    startsTime = ckt->CKTstat->STATsolveTime;   \
    startlTime = ckt->CKTstat->STATloadTime;    \
    startkTime = ckt->CKTstat->STATsyncTime;    \
} while(0)

#define UPDATE_STATS(analysis) \
do { \
    ckt->CKTcurrentAnalysis = analysis; \
    ckt->CKTstat->STATtranTime += SPfrontEnd->IFseconds() - startTime; \
    ckt->CKTstat->STATtranIter += ckt->CKTstat->STATnumIter - startIters; \
    ckt->CKTstat->STATtranDecompTime += ckt->CKTstat->STATdecompTime - startdTime; \
    ckt->CKTstat->STATtranSolveTime += ckt->CKTstat->STATsolveTime - startsTime; \
    ckt->CKTstat->STATtranLoadTime += ckt->CKTstat->STATloadTime - startlTime; \
    ckt->CKTstat->STATtranSyncTime += ckt->CKTstat->STATsyncTime - startkTime; \
} while(0)


/* Define some useful macro */
#define HISTORY 1024
#define ERR 1e+30
#define GF_LAST 313


static int
DFT (long int, int, double *, double *, double *, double, double *, double *, double *, double *, double *) ;

int
DCpss (CKTcircuit *ckt, int restart)
{
    int i, j, converged, error, firsttime, startIters, oscnNode ;
    double olddelta, delta, newdelta ;
    double startdTime, startsTime, startlTime, startkTime, startTime ;

    int save_order, numNames, ltra_num ;
    long save_mode ;
    IFuid timeUid ;
    IFuid freqUid ;
    IFuid *nameList ;
    double maxstepsize = 0.0 ;
    CKTnode *node ;

#ifdef XSPICE
/* gtri - add - wbk - 12/19/90 - Add IPC stuff */
    Ipc_Boolean_t ipc_firsttime = IPC_TRUE ;
    Ipc_Boolean_t ipc_secondtime = IPC_FALSE ;
    Ipc_Boolean_t ipc_delta_cut = IPC_FALSE ;
    double ipc_last_time = 0.0 ;
    double ipc_last_delta = 0.0 ;
/* gtri - end - wbk - 12/19/90 - Add IPC stuff */
#endif

#ifdef CLUSTER
    int redostep ;
#endif

    /* New variables */
    int in_stabilization, in_pss ;
    double err = 0, predsum = 0, diff = 0 ;
    double time_temp = 0, gf_history [HISTORY], rr_history [HISTORY], predsum_history [HISTORY], nextstep ;
    int msize, shooting_cycle_counter = 0, k = 0, flag_conv = 0 ;
    double *RHS_copy_se, *RHS_copy_der, *RHS_derivative, *pred, err_0 = ERR ;
    double time_err_min_1 = 0, time_err_min_0 = 0, err_min_0 = ERR, err_min_1 = 0 ;
    double err_1 = 0, err_max = ERR ;
    int pss_points_cycle = 0, dynamic_test = 0 ;
    double gf_last_0 = ERR, gf_last_1 = GF_LAST ;
    double thd = 0 ;
    double *psstimes, *pssvalues, *pssValues, *pssfreqs, *pssmags, *pssphases, *pssnmags, *pssnphases, *pssResults ;
    double *RHS_max, *RHS_min, *err_conv, *appMatrix, *oldMatrix ;
//    double err_conv_ref = 0, tv_01 = 0, *S_old, *S_diff ;
//    double *mulMatrix ;

    /* Francesco Lannutti's MOD */
    /* Stuff needed by frequency estimation reiteration, based on the DFT result */
    int position;
    double max_freq;


    /* Print some useful information */
    fprintf (stderr, "Periodic Steady State Analysis Started\n\n") ;
    fprintf (stderr, "PSS Guessed Frequency %g\n", ckt->CKTguessedFreq) ;
    fprintf (stderr, "PSS Points %ld\n", ckt->CKTpsspoints) ;
    fprintf (stderr, "PSS Harmonics number %d\n", ckt->CKTharms) ;
    fprintf (stderr, "PSS Steady Coefficient %g\n", ckt->CKTsteady_coeff) ;
    fprintf (stderr, "PSS sc_iter %d\n", ckt->CKTsc_iter) ;
    fprintf (stderr, "PSS Stabilization Time %g\n", ckt->CKTstabTime) ;


    /* Grab the current job */
    PSSan *job = (PSSan *) ckt->CKTcurJob ;
    oscnNode = job->PSSoscNode->number ;


    /* Variables and memory initialization */
    in_pss = 0 ;
    in_stabilization = 1 ;

    for (i = 0 ; i < HISTORY ; i++)
    {
        rr_history [i] = 0.0 ;
        gf_history [i] = 0.0 ;
    }

    msize = SMPmatSize (ckt->CKTmatrix) ;
    RHS_copy_se = TMALLOC (double, msize) ;  /* Set the current RHS reference for next Shooting Evaluation */
    RHS_copy_der = TMALLOC (double, msize) ; /* Used to compute current Derivative */
    RHS_derivative = TMALLOC (double, msize) ;
    pred = TMALLOC (double, msize) ;
    RHS_max = TMALLOC (double, msize) ;
    RHS_min = TMALLOC (double, msize) ;
    err_conv = TMALLOC (double, msize) ;
//    S_old = TMALLOC (double, msize) ;
//    S_diff = TMALLOC (double, msize) ;
    appMatrix = TMALLOC (double, msize*msize) ;
    oldMatrix = TMALLOC (double, msize*msize) ;
//    mulMatrix = TMALLOC (double, msize*msize) ;
    
    for (i = 0 ; i < msize ; i++)
    {
        RHS_copy_se [i] = 0.0 ;
        RHS_copy_der [i] = 0.0 ;
        RHS_derivative [i] = 0.0 ;
        pred [i] = 0.0 ;
        RHS_max [i] = 0.0 ;
        RHS_min [i] = 0.0 ;
//        S_old [i] = 0.0 ;
//        S_diff [i] = 0.0 ;
        for (j = 0 ; j < msize ; j++)
        { /* this is for matrices */
            appMatrix [msize * i + j] = 0.0 ;
            oldMatrix [msize * i + j] = 0.0 ;
//            mulMatrix [msize * i + j] = 0.0 ;
        }
    }

    psstimes = TMALLOC (double, ckt->CKTpsspoints + 1) ;
    pssvalues = TMALLOC (double, msize * (ckt->CKTpsspoints + 1)) ;
    pssValues = TMALLOC (double, ckt->CKTpsspoints + 1) ;
    pssfreqs = TMALLOC (double, ckt->CKTharms) ;
    pssmags = TMALLOC (double, ckt->CKTharms) ;
    pssphases = TMALLOC (double, ckt->CKTharms) ;
    pssnmags = TMALLOC (double, ckt->CKTharms) ;
    pssnphases = TMALLOC (double, ckt->CKTharms) ;
    pssResults = TMALLOC (double, msize * ckt->CKTharms) ;

    for (i = 0 ; i < ckt->CKTharms ; i++)
    {
        pssfreqs [i] = 0.0 ;
        pssmags [i] = 0.0 ;
        pssphases [i] = 0.0 ;
        pssnmags [i] = 0.0 ;
        pssnphases [i] = 0.0 ;
    }

    for (i = 0 ; i < msize * ckt->CKTharms ; i++)
        pssResults [i] = 0.0 ;

    for (i = 0 ; i < ckt->CKTpsspoints + 1 ; i++)
    {
        psstimes [i] = 0.0 ;
        pssValues [i] = 0.0 ;
    }

    for (i = 0 ; i < msize * (ckt->CKTpsspoints + 1) ; i++)
        pssvalues [i] = 0.0 ;

    /* Delta timestep and circuit time setup */
    delta = ckt->CKTstep ;
    ckt->CKTtime = ckt->CKTinitTime ;
    ckt->CKTfinalTime = ckt->CKTstabTime ;

    /* Starting PSS Algorithm, based on Transient Analysis */
    if (restart || ckt->CKTtime == 0)
    {
        delta = MIN (1 / ckt->CKTguessedFreq / 100, ckt->CKTstep) ;

#ifdef STEPDEBUG
        fprintf (stderr, "delta = %g    finalTime/200: %g    CKTstep: %g\n", delta, ckt->CKTfinalTime / 200, ckt->CKTstep) ;
#endif

        /* begin LTRA code addition */
        if (ckt->CKTtimePoints != NULL)
            FREE (ckt->CKTtimePoints) ;

        if (ckt->CKTstep >= ckt->CKTmaxStep)
            maxstepsize = ckt->CKTstep ;
        else
            maxstepsize = ckt->CKTmaxStep ;

        ckt->CKTsizeIncr = 10 ;
        ckt->CKTtimeIndex = -1 ; /* before the DC solution has been stored */
        ckt->CKTtimeListSize = (int)(1 / ckt->CKTguessedFreq / maxstepsize + 0.5) ;
        ltra_num = CKTtypelook ("LTRA") ;
        if (ltra_num >= 0 && ckt->CKThead [ltra_num] != NULL)
            ckt->CKTtimePoints = NEWN (double, ckt->CKTtimeListSize) ;
        /* end LTRA code addition */

        /* Breakpoints initialization */
        if (ckt->CKTbreaks)
            FREE (ckt->CKTbreaks) ;
        ckt->CKTbreaks = TMALLOC (double, 2) ;
        if (ckt->CKTbreaks == NULL)
            return (E_NOMEM) ;
        ckt->CKTbreaks [0] = 0 ;
        ckt->CKTbreaks [1] = ckt->CKTfinalTime ;
        ckt->CKTbreakSize = 2 ;

#ifdef XSPICE
/* gtri - begin - wbk - 12/19/90 - Modify setting of CKTminBreak */
        /* if (ckt->CKTminBreak == 0)
               ckt->CKTminBreak = ckt->CKTmaxStep * 5e-5 ; */

        /* Set to 10 times delmin for ATESSE 1 compatibity */
        if (ckt->CKTminBreak == 0)
            ckt->CKTminBreak = 10.0 * ckt->CKTdelmin ;
/* gtri - end - wbk - 12/19/90 - Modify setting of CKTminBreak */
#else
        /* Minimum Breakpoint Setup */
        if (ckt->CKTminBreak == 0)
            ckt->CKTminBreak = ckt->CKTmaxStep * 5e-5 ;
#endif

#ifdef XSPICE
/* gtri - add - wbk - 12/19/90 - Add IPC stuff and set anal_init and anal_type */
        /* Tell the beginPlot routine what mode we're in */
        g_ipc.anal_type = IPC_ANAL_TRAN ;

        /* Tell the code models what mode we're in */
        g_mif_info.circuit.anal_type = MIF_DC ;

        g_mif_info.circuit.anal_init = MIF_TRUE ;
/* gtri - end - wbk */
#endif

	/* Time Domain plot start and prepared to be filled in later */
        error = CKTnames (ckt, &numNames, &nameList) ;
        if (error)
            return (error) ;
        SPfrontEnd->IFnewUid (ckt, &timeUid, NULL, "time", UID_OTHER, NULL) ;
        error = SPfrontEnd->OUTpBeginPlot (ckt, ckt->CKTcurJob,
            "Time Domain Periodic Steady State Analysis",
            timeUid, IF_REAL, numNames, nameList, IF_REAL, &(job->PSSplot_td)) ;
        tfree (nameList) ;
        if (error)
            return(error) ;

        /* Time initialization for Transient Analysis */
        ckt->CKTtime = 0 ;
        ckt->CKTdelta = 0 ;
        ckt->CKTbreak = 1 ;
        firsttime = 1 ;
        save_mode = (ckt->CKTmode&MODEUIC) | MODETRANOP | MODEINITJCT ;
        save_order = ckt->CKTorder ;

#ifdef XSPICE
/* gtri - begin - wbk - set a breakpoint at end of supply ramping time */
        /* must do this after CKTtime set to 0 above */
        if (ckt->enh->ramp.ramptime > 0.0)
            CKTsetBreak (ckt, ckt->enh->ramp.ramptime) ;
/* gtri - end - wbk - set a breakpoint at end of supply ramping time */

/* gtri - begin - wbk - Call EVTop if event-driven instances exist */
        if (ckt->evt->counts.num_insts != 0)
        {
	    /* use new DCOP algorithm */
            converged = EVTop (ckt, (ckt->CKTmode & MODEUIC) | MODETRANOP | MODEINITJCT,
                               (ckt->CKTmode & MODEUIC) | MODETRANOP | MODEINITFLOAT, ckt->CKTdcMaxIter, MIF_TRUE);
            EVTdump (ckt, IPC_ANAL_DCOP, 0.0) ;

            EVTop_save (ckt, MIF_FALSE, 0.0) ;

        } else
/* gtri - end - wbk - Call EVTop if event-driven instances exist */
#endif

        /* Looking for a working Operating Point */
        converged = CKTop (ckt, (ckt->CKTmode & MODEUIC) | MODETRANOP | MODEINITJCT,
                           (ckt->CKTmode & MODEUIC) | MODETRANOP | MODEINITFLOAT, ckt->CKTdcMaxIter) ;

#ifdef STEPDEBUG
        if (converged != 0)
        {
            fprintf (stdout, "\nTransient solution failed -\n") ;
            CKTncDump (ckt) ;
            fprintf (stdout, "\n") ;
            fflush (stdout) ;
        }
        else if (!ft_noacctprint && !ft_noinitprint)
        {
            fprintf (stdout, "\nInitial Transient Solution\n") ;
            fprintf (stdout, "--------------------------\n\n") ;
            fprintf (stdout, "%-30s %15s\n", "Node", "Voltage") ;
            fprintf (stdout, "%-30s %15s\n", "----", "-------") ;
            for (node = ckt->CKTnodes->next ; node ; node = node->next)
            {
                if (strstr (node->name, "#branch") || !strstr (node->name, "#"))
                    fprintf(stdout, "%-30s %15g\n", node->name, ckt->CKTrhsOld [node->number]) ;
            }
            fprintf (stdout, "\n") ;
            fflush (stdout) ;
        }
#endif

        /* If no convergence reached - NO valid Operating Point */
        if (converged != 0)
            return (converged) ;

#ifdef XSPICE
/* gtri - add - wbk - 12/19/90 - Add IPC stuff */
        /* Send the operating point results for Mspice compatibility */
        if (g_ipc.enabled)
        {
            ipc_send_dcop_prefix () ;
            CKTdump (ckt, 0.0, job->PSSplot_td) ;
            ipc_send_dcop_suffix () ;
        }
/* gtri - end - wbk */

/* gtri - add - wbk - 12/19/90 - set anal_init and anal_type */
        g_mif_info.circuit.anal_init = MIF_TRUE ;

        /* Tell the code models what mode we're in */
        g_mif_info.circuit.anal_type = MIF_TRAN ;
/* gtri - end - wbk */

/* gtri - begin - wbk - Add Breakpoint stuff */
        /* Initialize the temporary breakpoint variables to infinity */
        g_mif_info.breakpoint.current = ERR ;
        g_mif_info.breakpoint.last    = ERR ;

/* gtri - end - wbk - Add Breakpoint stuff */
#endif

        ckt->CKTstat->STATtimePts++ ;

        /* Setting Integration Order to Backward Euler */
        ckt->CKTorder = 1 ;

        /* Copying the maxStep to every deltaOld */
        for (i = 0 ; i < 7 ; i++)
            ckt->CKTdeltaOld [i] = ckt->CKTmaxStep ;

        /* Setting DELTA */
        ckt->CKTdelta = delta ;

#ifdef STEPDEBUG
        fprintf (stderr, "delta initialized to %g\n", ckt->CKTdelta) ;
#endif

	ckt->CKTsaveDelta = ckt->CKTfinalTime / 50 ;

        /* Changing Circuit MODE */
	/* modeinittran set here */
        ckt->CKTmode = (ckt->CKTmode & MODEUIC) | MODETRAN | MODEINITTRAN ;
        ckt->CKTag [0] = 0 ;
        ckt->CKTag [1] = 0 ;

        /* State0 copied into State1 - DEPRECATED LEGACY function - to be replaced with memmove() */
        bcopy (ckt->CKTstate0, ckt->CKTstate1, (size_t) ckt->CKTnumStates * sizeof(double)) ;

        /* Statistics Initialization using a macro at the beginning of this code */
        INIT_STATS () ;

#ifdef CLUSTER
        CLUsetup (ckt) ;
#endif

    } else {
        /* saj As traninit resets CKTmode */
        ckt->CKTmode = (ckt->CKTmode&MODEUIC) | MODETRAN | MODEINITPRED ;

        /* saj */
        INIT_STATS () ;

        if (ckt->CKTminBreak == 0)
            ckt->CKTminBreak = ckt->CKTmaxStep * 5e-5 ;

        firsttime = 0 ;

        /* To get rawfile working saj*/
        error = SPfrontEnd->OUTpBeginPlot (NULL, NULL, NULL, NULL, 0, 666, NULL, 666, &(job->PSSplot_td)) ;
        if (error)
        {
            fprintf (stderr, "Couldn't relink rawfile\n") ;
            return error ;
        }
        /*end saj*/

        /* Skip nextTime if it isn't the firsttime! :) */
        goto resume ;
    }

    /* nextTime */
nextTime:

    /* begin LTRA code addition */
    if (ckt->CKTtimePoints)
    {
        ckt->CKTtimeIndex++ ;
        if (ckt->CKTtimeIndex >= ckt->CKTtimeListSize)
        {
            /* need more space */
            int need ;
            if (in_stabilization)
                need = (int)(0.5 + (ckt->CKTstabTime - ckt->CKTtime) / maxstepsize) ; /* FIXME, ceil ? */
            else
                need = (int)(0.5 + (time_temp + 1 / ckt->CKTguessedFreq - ckt->CKTtime) / maxstepsize) ;

            if (need < ckt->CKTsizeIncr)
                need = ckt->CKTsizeIncr ;

            ckt->CKTtimeListSize += need ;
            ckt->CKTtimePoints = TREALLOC (double, ckt->CKTtimePoints, ckt->CKTtimeListSize) ;
            ckt->CKTsizeIncr = (int) ceil (1.4 * ckt->CKTsizeIncr) ;
        }
        ckt->CKTtimePoints [ckt->CKTtimeIndex] = ckt->CKTtime ;
    }
    /* end LTRA code addition */

    /* Check for the timepoint acceptance */
    error = CKTaccept (ckt) ;

    /* Check if the current breakpoint is outdated; if yes, clear */
    if (ckt->CKTtime > ckt->CKTbreaks [0])
        CKTclrBreak (ckt) ;

    /*
    * Breakpoint handling scheme:
    * When a timepoint t is accepted (by CKTaccept), clear all previous
    * breakpoints, because they will never be needed again.
    *
    * t may itself be a breakpoint, or indistinguishably close. DON'T
    * clear t itself; recognize it as a breakpoint and act accordingly
    *
    * if t is not a breakpoint, limit the timestep so that the next
    * breakpoint is not crossed
    */

#ifdef STEPDEBUG
    fprintf (stderr, "Delta %g accepted at time %g (finaltime: %g)\n", ckt->CKTdelta, ckt->CKTtime, ckt->CKTfinalTime) ;
    fflush (stdout) ;
#endif

    ckt->CKTstat->STATaccepted++ ;
    ckt->CKTbreak = 0 ;

    /* XXX Error will cause single process to bail. */
    if (error) {
        UPDATE_STATS (DOING_TRAN);
        return (error);
    }

#ifdef XSPICE
/* gtri - modify - wbk - 12/19/90 - Send IPC stuff */
    if (g_ipc.enabled)
    {
        if (in_pss)
        {
            /* Send event-driven results */
            EVTdump (ckt, IPC_ANAL_TRAN, 0.0) ;

            /* Then follow with analog results... */

            /* Test to see if delta was cut by a breakpoint, */
            /* a non-convergence, or a too large truncation error */
            if (ipc_firsttime)
                ipc_delta_cut = IPC_FALSE ;
            else if (ckt->CKTtime < (ipc_last_time + (0.999 * ipc_last_delta)))
                ipc_delta_cut = IPC_TRUE ;
            else
                ipc_delta_cut = IPC_FALSE ;

            /* Record the data required to check for delta cuts */
            ipc_last_time = ckt->CKTtime ;
            ipc_last_delta = MIN (ckt->CKTdelta, ckt->CKTmaxStep) ;

            /* Send results data if time since last dump is greater */
            /* than 'mintime', or if first or second timepoints, */
            /* or if delta was cut */
            if ((ckt->CKTtime >= (g_ipc.mintime + g_ipc.last_time)) || ipc_firsttime || ipc_secondtime || ipc_delta_cut)
            {
                ipc_send_data_prefix (ckt->CKTtime) ;
                CKTdump (ckt, ckt->CKTtime, job->PSSplot_td) ;
                ipc_send_data_suffix () ;

                if (ipc_firsttime)
                {
                    ipc_firsttime = IPC_FALSE ;
                    ipc_secondtime = IPC_TRUE ;
                }
                else if (ipc_secondtime)
                    ipc_secondtime = IPC_FALSE ;

                g_ipc.last_time = ckt->CKTtime;
            }
        }
    } else
/* gtri - modify - wbk - 12/19/90 - Send IPC stuff */
#endif

#ifdef CLUSTER
    if (in_pss)
        CLUoutput (ckt) ;
#endif

    if (in_pss)
    {
        nextstep = time_temp + 1 / ckt->CKTguessedFreq * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints) ;

        /* If in_pss, store data for Time Domain Plot and gather ordered data for FFT computing */
	if ((AlmostEqualUlps (ckt->CKTtime, nextstep, 10)) || (ckt->CKTtime > time_temp + 1 / ckt->CKTguessedFreq))
        {

#ifdef STEPDEBUG
            fprintf (stderr, "IN_PSS: time point accepted in evolution for FFT calculations.\n") ;
            fprintf (stderr, "Circuit time %1.15g, final time %1.15g, point index %d and total requested points %ld\n",
                     ckt->CKTtime, nextstep, pss_points_cycle, ckt->CKTpsspoints) ;
#endif

            CKTdump (ckt, ckt->CKTtime, job->PSSplot_td) ;

            /* Store the Time Base for the DFT */
            psstimes [pss_points_cycle] = ckt->CKTtime ;

            /* Store values for the FFT calculation */
            for (i = 1 ; i <= msize ; i++)
                pssvalues [i - 1 + pss_points_cycle * msize] = ckt->CKTrhsOld [i] ;

            /* Update PSS counter cycle, used to stop the entire algorithm */
            pss_points_cycle++ ;

            /* Set the next BreakPoint for PSS */
            CKTsetBreak (ckt, time_temp + (1 / ckt->CKTguessedFreq) * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints)) ;

#ifdef STEPDEBUG
            fprintf (stderr, "Next breakpoint set in: %1.15g\n", time_temp + 1 / ckt->CKTguessedFreq * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints)) ;
#endif

        } else { 
            /* Algo can enter here but should do nothing */

#ifdef STEPDEBUG
            fprintf (stderr, "IN_PSS: time point accepted in evolution but dropped for FFT calculations\n") ;
#endif

        }
    }

#ifdef XSPICE
/* gtri - begin - wbk - Update event queues/data for accepted timepoint */
    /* Note: this must be done AFTER sending results to SI so it can't */
    /* go next to CKTaccept() above */
    if (ckt->evt->counts.num_insts > 0)
        EVTaccept (ckt, ckt->CKTtime) ;
/* gtri - end - wbk - Update event queues/data for accepted timepoint */
#endif

    ckt->CKTstat->STAToldIter = ckt->CKTstat->STATnumIter ;

    /* ***********************************/
    /* ******* SHOOTING CODE BLOCK *******/
    /* ***********************************/
    if (in_stabilization)
    {
        /* Test if stabTime has been reached */
        if (AlmostEqualUlps (ckt->CKTtime, ckt->CKTstabTime, 100))
        {
            time_temp = ckt->CKTtime ;

            /* Are you sure about 2 times the Period ? */
            ckt->CKTfinalTime = time_temp + 2 / ckt->CKTguessedFreq ;

            /* Set the first requested breakpoint - Maximum of Shooting? - Same consideration as before */
            CKTsetBreak (ckt, time_temp + 1 / ckt->CKTguessedFreq) ;
            fprintf (stderr, "Exiting from stabilization\n") ;
            fprintf (stderr, "Time of first shooting evaluation will be %1.10g\n", time_temp + 1 / ckt->CKTguessedFreq) ;

            /* Next time is no more in stabilization - Unset the flag */
            in_stabilization = 0 ;

            /* Save the RHS_copy_der as the NEW CKTrhsOld */
            for (i = 1 ; i <= msize ; i++)
                RHS_copy_der [i - 1] = ckt->CKTrhsOld [i] ;

            /* Print RHS on exiting from stabilization */
            fprintf (stderr, "RHS on exiting from stabilization: ") ;
            for (i = 1 ; i <= msize ; i++)
            {
                RHS_copy_se [i - 1] = ckt->CKTrhsOld [i] ;
                fprintf (stderr, "%-15g ", RHS_copy_se [i - 1]) ;
            }
            fprintf (stderr, "\n") ;
	}
    }
    /* ELSE not in stabilization but in Shooting */
    else if ((!in_pss) && (!in_stabilization))
    {
        /* Calculation of error norms of RHS solution of every accepted nextTime */
        err = 0 ;
        predsum = 0 ;
        for (i = 0 ; i < msize ; i++)
        {

            /* Save max per node or branch of every estimated period */
            if (RHS_max [i] < ckt->CKTrhsOld [i + 1])
                RHS_max [i] = ckt->CKTrhsOld [i + 1] ;

            /* Save min per node or branch of every estimated period */
            if (RHS_min [i] > ckt->CKTrhsOld [i + 1])
                RHS_min [i] = ckt->CKTrhsOld [i + 1] ;

            /* CKTrhsOld is the last CORRECT value of RHS */
            diff = ckt->CKTrhsOld [i + 1] - RHS_copy_se [i] ;
            err_conv [i] = diff ;
            err += diff * diff ;

            /* Compute and store derivative */
            RHS_derivative [i] = (ckt->CKTrhsOld [i + 1] - RHS_copy_der [i]) / ckt->CKTdelta ;

            /* Check if derivative is bounded by maximum allowable derivative */
            if (fabs (RHS_derivative [i]) > 2 * 3.14159 * ckt->CKTguessedFreq * MAX (fabs (RHS_max [i]), fabs (RHS_min [i])))
            {
                if (RHS_derivative [i] < 0)
                    RHS_derivative [i] = - 3.14159 * ckt->CKTguessedFreq * MAX (fabs (RHS_max [i]), fabs (RHS_min [i])) ;
                else
                    RHS_derivative [i] = 3.14159 * ckt->CKTguessedFreq * MAX (fabs (RHS_max [i]), fabs (RHS_min [i])) ;
            } 

            /* Save the RHS_copy_der as the NEW CKTrhsOld */
            RHS_copy_der [i] = ckt->CKTrhsOld [i + 1] ;

            /* Pitagora ha sempre ragione!!! :))) */
            /* pred is treated as FREQUENCY to avoid numerical overflow when derivative is close to ZERO */
            pred [i] = RHS_derivative [i] / diff ;

#ifdef STEPDEBUG
            fprintf (stderr, "Pred is so high or so low! Diff is: %g\n", diff) ;
#endif

            if ((fabs (pred [i]) > 1.0e6 * ckt->CKTguessedFreq) || (diff == 0))
            {
                if (pred [i] > 0)
                    pred [i] = 1.0e6 * ckt->CKTguessedFreq ;
                else
                    pred [i] = -1.0e6 * ckt->CKTguessedFreq ;
            }

            predsum += (1 / pred [i]) ;

#ifdef STEPDEBUG
            fprintf (stderr, "Predsum in time before to be divided by dynamic_test has value %g\n", predsum) ;
            fprintf (stderr, "Current Diff: %g, Derivative: %g, Frequency Projection: %g\n", diff, RHS_derivative [i], pred [i]) ;
#endif

        }
        err = sqrt (err) ;

        /* Start frequency estimation */
        if ((err < err_0) && (ckt->CKTtime >= time_temp + 0.5 / ckt->CKTguessedFreq)) /* far enough from time temp... */
        {
            if (err < err_min_0)
            {
                err_min_1 = err_min_0 ;            /* store previous minimum of RHS vector error */
                err_min_0 = err ;                  /* store minimum of RHS vector error */
                time_err_min_1 = time_err_min_0 ;  /* store previous minimum of RHS vector error time */
                time_err_min_0 = ckt->CKTtime ;    /* store minimum of RHS vector error time */
            }
        }
        err_0 = err ;

        if ((err > err_1) && (ckt->CKTtime >= time_temp + 0.1 / ckt->CKTguessedFreq)) /* far enough from time temp... */
        {
            if (err > err_max)
                err_max = err ;                /* store maximum of RHS vector error */
        }
        err_1 = err ;

        /* If evolution is near shooting... */
        if ((AlmostEqualUlps (ckt->CKTtime, time_temp + 1 / ckt->CKTguessedFreq, 10)) || (ckt->CKTtime > time_temp + 1 / ckt->CKTguessedFreq))
        {
            flag_conv = 0 ;

            if (shooting_cycle_counter == 0)
            {
                /* If first time in shooting we warn about that ! */
                fprintf (stderr, "In shooting...\n") ;
            }

            /* For debugging purpose */
            fprintf (stderr, "\n----------------\n") ;
            fprintf (stderr, "Shooting cycle iteration number: %3d ||", shooting_cycle_counter) ;

            if (shooting_cycle_counter > 0)
                fprintf (stderr, " rr: %g || predsum: %g\n", rr_history [shooting_cycle_counter - 1], predsum) ;
            else
                fprintf (stderr, " rr: %g || predsum: %g\n", 0.0, predsum) ;
            /* --------------------- */

//            fprintf (stderr, "Print of dynamically consistent nodes voltages or branches currents:\n") ;

            for (i = 0, node = ckt->CKTnodes->next ; node ; i++, node = node->next)
            {
                /* Voltage Node */
                if (!strstr (node->name, "#"))
                {
//                    tv_01 = MAX (fabs (RHS_max [i]), fabs (RHS_min [i])) ;

//                    err_conv_ref += ((RHS_max [i] - RHS_min [i]) * ckt->CKTreltol + ckt->CKTvoltTol) * ckt->CKTtrtol * ckt->CKTsteady_coeff ;

                    if (fabs (err_conv [i]) > (fabs (RHS_max [i] - RHS_min [i]) * ckt->CKTreltol + ckt->CKTvoltTol) * ckt->CKTtrtol * ckt->CKTsteady_coeff)
                        flag_conv++ ;

                    /* If the dynamic is below 10uV, it's dropped */
                    if (fabs (RHS_max [i] - RHS_min [i]) > 10 * 1e-6)
                    {
//                        S_diff [i] = 2 - ((RHS_max [i] - RHS_min [i]) / tv_01 - S_old [i]) ;
//                        S_old [i] = (RHS_max [i] - RHS_min [i]) / tv_01 ;
//                        if (fabs (S_old [i]) > 0.1)
//                            fprintf (stderr, "[V] %20s: RHSd %-12g || ECR %-12g || RHSM %-12g || RHSm %-12g || Dynamic variation %-12g || prediction %1.10lg\n",
//                                     node->name, ckt->CKTrhsOld [i + 1] - RHS_copy_se [i],
//                                     ((RHS_max [i] - RHS_min [i - 1]) * ckt->CKTreltol + 1e-6) * ckt->CKTtrtol * ckt->CKTsteady_coeff,
//                                     RHS_max [i], RHS_min [i], S_diff [i], 1 / pred [i]) ;
                        dynamic_test++ ; /* test on voltage dynamic consistence */
//                    } else {
//                        S_old [i] = 0 ;
//                        S_diff [i] = 0 ;
                    }

                /* Current Node */
                } else {
//                    tv_01 = MAX (fabs (RHS_max [i]), fabs (RHS_min [i])) ;

//                    err_conv_ref += ((RHS_max [i] - RHS_min [i]) * ckt->CKTreltol + ckt->CKTabstol) * ckt->CKTtrtol * ckt->CKTsteady_coeff ;

                    if (fabs (err_conv [i]) > (fabs (RHS_max [i] - RHS_min [i]) * ckt->CKTreltol + ckt->CKTabstol) * ckt->CKTtrtol * ckt->CKTsteady_coeff)
                        flag_conv++ ;

                    /* If the dynamic is below 10nA, it's dropped */
                    if (fabs (RHS_max [i] - RHS_min [i]) > 10 * 1e-9)
                    {
//                        S_diff [i] = 2 - ((RHS_max [i] - RHS_min [i]) / tv_01 - S_old [i]) ;
//                        S_old [i] = (RHS_max [i] - RHS_min [i]) / tv_01 ;
//                        if (fabs (S_old [i]) > 0.1)
//                            fprintf (stderr, "[I] %20s: RHSd %-12g || ECR %-12g || RHSM %-12g || RHSm %-12g || Dynamic variation %-12g || prediction %1.10lg\n",
//                                     node->name, ckt->CKTrhsOld [i + 1] - RHS_copy_se [i],
//                                     ((RHS_max [i] - RHS_min [i]) * ckt->CKTreltol + 1e-9) * ckt->CKTtrtol * ckt->CKTsteady_coeff,
//                                     RHS_max [i], RHS_min [i], S_diff [i], 1/pred [i]) ;
                        dynamic_test++ ; /* test on current dynamic consistence */
//                    } else {
//                        S_old [i] = 0 ;
//                        S_diff [i] = 0 ;
                    }
                }
            }

            /* Take the mean value of time prediction trough the dynamic test variable */
            predsum /= dynamic_test ;

            /* Store the predsum history as absolute value */
            predsum_history [shooting_cycle_counter] = fabs (predsum) ;

            if (dynamic_test == 0)
            {
                /* Test for dynamic existence */
                fprintf (stderr, "No detectable dynamic on voltages nodes or currents branches. PSS analysis aborted\n") ;

                /* Terminates plot in Time Domain and frees the allocated memory */
                SPfrontEnd->OUTendPlot (job->PSSplot_td) ;
                FREE (RHS_copy_se) ;
                FREE (RHS_copy_der) ;
                FREE (RHS_max) ;
                FREE (RHS_min) ;
//                FREE (S_old) ;
//                FREE (S_diff) ;
                FREE (pssfreqs) ;
                FREE (psstimes) ;
                FREE (pssValues) ;
                FREE (pssResults) ;
                FREE (pssmags) ;
                FREE (pssphases) ;
                FREE (pssnmags) ;
                FREE (pssnphases) ;
                FREE (appMatrix) ;
                FREE (oldMatrix) ;
//                FREE (mulMatrix) ;
                return (E_PANIC) ; /* to be corrected with definition of new error macro in iferrmsg.h */
            }
            else if ((time_err_min_0 - time_temp) < 0)
            {
                /* Something has gone wrong... */
                fprintf (stderr, "Cannot find a minimum for error vector in estimated period. Try to adjust tstab! PSS analysis aborted\n") ;

                /* Terminates plot in Time Domain and frees the allocated memory */
                SPfrontEnd->OUTendPlot (job->PSSplot_td) ;
                FREE (RHS_copy_se) ;
                FREE (RHS_copy_der) ;
                FREE (RHS_max) ;
                FREE (RHS_min) ;
//                FREE (S_old) ;
//                FREE (S_diff) ;
                FREE (pssfreqs) ;
                FREE (psstimes) ;
                FREE (pssValues) ;
                FREE (pssResults) ;
                FREE (pssmags) ;
                FREE (pssphases) ;
                FREE (pssnmags) ;
                FREE (pssnphases) ;
                FREE (appMatrix) ;
                FREE (oldMatrix) ;
//                FREE (mulMatrix) ;
                return (E_PANIC) ; /* to be corrected with definition of new error macro in iferrmsg.h */
            }

//#ifdef STEPDEBUG
//            fprintf (stderr, "Global Convergence Error reference: %g, Time Projection: %g.\n",
//                     err_conv_ref / dynamic_test, predsum) ;
//#endif

            /***********************************/
            /*** FREQUENCY ESTIMATION UPDATE ***/
            /***********************************/
            if ((err_min_0 == err) || (err_min_0 == ERR))
            {
                /* Enters here if guessed frequency is higher than the 'real' value */
                ckt->CKTguessedFreq = 1 / (1 / ckt->CKTguessedFreq + fabs (predsum)) ;
                
#ifdef STEPDEBUG
                fprintf (stderr, "Frequency DOWN: est per %g, err min %g, err min 1 %g, err max %g, err %g\n",
                         time_err_min_0 - time_temp, err_min_0, err_min_1, err_max, err) ;
#endif

                /* Temporary variables to store previous occurrence of guessed frequency */
                gf_last_1 = gf_last_0 ;
                gf_last_0 = ckt->CKTguessedFreq ;
            } else {

                /* Enters here if guessed frequency is lower than the 'real' value */
                ckt->CKTguessedFreq = 1 / (time_err_min_0 - time_temp) ;

#ifdef STEPDEBUG
                fprintf (stderr, "Frequency UP:  est per %g, err min %g, err min 1 %g, err max %g, err %g\n",
                         time_err_min_0 - time_temp, err_min_0, err_min_1, err_max, err) ;
#endif

                gf_last_1 = gf_last_0 ;
                gf_last_0 = ckt->CKTguessedFreq ;
            }

            /* Next evaluation of shooting will be updated time (time_temp) summed to updated guessed period */
            time_temp = ckt->CKTtime ;

            /* IMPORTANT! Final time must be updated! Otherwise delta time can be wrongly calculated */
            ckt->CKTfinalTime = time_temp + 2 / ckt->CKTguessedFreq ;

            /* Set next the breakpoint */
            CKTsetBreak (ckt, time_temp + 1 / ckt->CKTguessedFreq) ;

            /* Store error history */
            rr_history [shooting_cycle_counter] = err ;
            gf_history [shooting_cycle_counter] = ckt->CKTguessedFreq ;
            shooting_cycle_counter++ ;

            fprintf (stderr, "Updated guessed frequency: %1.10lg .\n", ckt->CKTguessedFreq) ;
            fprintf (stderr, "Next shooting evaluation time is %1.10g and current time is %1.10g.\n",
                     time_temp + 1 / ckt->CKTguessedFreq, ckt->CKTtime) ;

            /* Shooting Exit Condition */
            if ((shooting_cycle_counter > ckt->CKTsc_iter) || (flag_conv == 0))
            {

#ifdef STEPDEBUG
                fprintf (stderr, "\nFrequency estimation (FE) and RHS period residual (PR) evolution\n") ;
#endif

//                err_0 = rr_history [0] ;
                err_0 = predsum_history [0] ;
                for (i = 0 ; i < shooting_cycle_counter ; i++)
                {
                    /* Print some statistics */
                    fprintf (stderr, "%-3d -> FE: %-15.10g || RR: %15.10g", i, gf_history [i], rr_history [i]) ;

                    /* Take the minimum residual iteration */
//                    if (err_0 > rr_history [i])
                    if (err_0 > predsum_history [i])
                    {
//                        err_0 = rr_history [i] ;
                        err_0 = predsum_history [i] ;
                        k = i ;
                    }
                    fprintf (stderr, " || err_0: %15.10g || predsum/dynamic_test: %15.10g\n", err_0, predsum_history [i]) ;
                }

                if (flag_conv == 0)
                {
                    /* PERIODIC STEADY STATE REACHED set the in_pss flag */
                    /* Flag the entering in PSS status */
                    in_pss = 1 ;

                    /* Update the last valid Guessed Frequency */
                    ckt->CKTguessedFreq = gf_history [shooting_cycle_counter - 1] ;

                    /* Save the current Time */
                    time_temp = ckt->CKTtime ;

                    /* Set the new Final Time */
                    ckt->CKTfinalTime = time_temp + 1 / ckt->CKTguessedFreq ;

                    /* Dump the first PSS point for the FFT */
                    CKTdump (ckt, ckt->CKTtime, job->PSSplot_td) ;
                    psstimes [pss_points_cycle] = ckt->CKTtime ;
                    for (i = 1 ; i <= msize ; i++)
                        pssvalues [i - 1 + pss_points_cycle * msize] = ckt->CKTrhsOld [i] ;

                    /* Update the PSS points counter and set the next Breakpoint */
                    pss_points_cycle++ ;
                    CKTsetBreak (ckt, time_temp + (1 / ckt->CKTguessedFreq) * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints)) ;

                    fprintf (stderr, "\nConvergence reached. Final circuit time is %1.10g seconds (iteration n° %d) and predicted fundamental frequency is %g Hz\n", ckt->CKTtime, shooting_cycle_counter - 1, ckt->CKTguessedFreq) ;

#ifdef STEPDEBUG
                    fprintf (stderr, "time_temp %g\n", time_temp) ;
                    fprintf (stderr, "IN_PSS: FIRST time point accepted in evolution for FFT calculations\n") ;
                    fprintf (stderr, "Circuit time %1.15g, final time %1.15g, point index %d and total requested points %ld\n",
                             ckt->CKTtime, time_temp + 1 / ckt->CKTguessedFreq * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints),
                             pss_points_cycle, ckt->CKTpsspoints) ;
                    fprintf (stderr, "Next breakpoint set in: %1.15g\n",
                             time_temp + 1 / ckt->CKTguessedFreq * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints)) ;
#endif

                } else {
                    /* PERIODIC STEADY STATE NOT REACHED - however set the in_pss flag */
                    /* Flag the entering in PSS status */
                    in_pss = 1 ;

                    /* Update the last valid Guessed Frequency */
                    ckt->CKTguessedFreq = gf_history [k] ; /* k problem, k could be zero !! */

                    /* Save the current Time */
                    time_temp = ckt->CKTtime ;

                    /* Set the new Final Time */
                    ckt->CKTfinalTime = time_temp + 1 / ckt->CKTguessedFreq ;

                    /* Dump the first PSS point for the FFT */
                    CKTdump (ckt, ckt->CKTtime, job->PSSplot_td) ;
                    psstimes [pss_points_cycle] = ckt->CKTtime ;
                    for (i = 1 ; i <= msize ; i++)
                        pssvalues [i - 1 + pss_points_cycle * msize] = ckt->CKTrhsOld [i] ;

                    /* Update the PSS points counter and set the next Breakpoint */
                    pss_points_cycle++ ;
                    CKTsetBreak (ckt, time_temp + (1 / ckt->CKTguessedFreq) * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints)) ;

                    fprintf (stderr, "\nConvergence not reached. However the most near convergence iteration has predicted (iteration %d) a fundamental frequency of %g Hz\n", k, ckt->CKTguessedFreq) ;

#ifdef STEPDEBUG
                    fprintf (stderr, "time_temp %g\n", time_temp) ;
                    fprintf (stderr, "IN_PSS: FIRST time point accepted in evolution for FFT calculations\n") ;
                    fprintf (stderr, "Circuit time %1.15g, final time %1.15g, point index %d and total requested points %ld\n",
                             ckt->CKTtime, time_temp + 1 / ckt->CKTguessedFreq * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints),
                             pss_points_cycle, ckt->CKTpsspoints) ;
                    fprintf (stderr, "Next breakpoint set in: %1.15g\n",
                             time_temp + 1 / ckt->CKTguessedFreq * ((double)(pss_points_cycle) / (double)ckt->CKTpsspoints)) ;
#endif

                }
            }

            /* Restore maximum and minimum error for next search */
            err_min_0 = ERR ;
            err_max = -ERR ;
            err_0 = ERR ;
            err_1 = -ERR ;
//            err_conv_ref = 0 ;
            dynamic_test = 0 ;

            /* Reset actual RHS reference for next shooting evaluation */
            for (i = 1 ; i <= msize ; i++)
                RHS_copy_se [i - 1] = ckt->CKTrhsOld [i] ;

#ifdef STEPDEBUG
            fprintf (stderr, "RHS on new shooting cycle: ") ;
            for (i = 0 ; i < msize ; i++)
                fprintf (stderr, "%-15g ", RHS_copy_se [i]) ;
            fprintf (stderr, "\n") ;
#endif

            if (in_pss != 1)
            {
                for (i = 0 ; i < msize ; i++)
                {
                    /* Reset max and min per node or branch on every shooting cycle */
//                    RHS_max [i] = -ERR ;
//                    RHS_min [i] = ERR ;
                    RHS_max [i] = 0 ;
                    RHS_min [i] = 0 ;
                }
            }
            fprintf (stderr, "----------------\n\n") ;
        }
    } else {
        /* The algorithm enters here when in_pss is set */

#ifdef STEPDEBUG
        fprintf (stderr, "ttemp %1.15g, final_time %1.15g, current_time %1.15g\n", time_temp, time_temp + 1 / ckt->CKTguessedFreq, ckt->CKTtime) ;
#endif

        if ((pss_points_cycle == ckt->CKTpsspoints + 1) || (ckt->CKTtime > ckt->CKTfinalTime))
        {
            /* End plot in Time Domain */
            SPfrontEnd->OUTendPlot (job->PSSplot_td) ;

            /* Frequency Plot Creation */
            error = CKTnames (ckt, &numNames, &nameList) ;
            if (error)
                return (error) ;
            SPfrontEnd->IFnewUid (ckt, &freqUid, NULL, "frequency", UID_OTHER, NULL) ;
            error = SPfrontEnd->OUTpBeginPlot (ckt, ckt->CKTcurJob, "Frequency Domain Periodic Steady State Analysis",
                                               freqUid, IF_REAL, numNames, nameList, IF_REAL, &(job->PSSplot_fd)) ;
            tfree (nameList) ;
            SPfrontEnd->OUTattributes (job->PSSplot_fd, NULL, PLOT_COMB, NULL) ;

            /* ******************** */
            /* Starting DFT on data */
            /* ******************** */
            for (i = 0 ; i < msize ; i++)
            {
                for (j = 0 ; j < ckt->CKTpsspoints ; j++)
                    pssValues [j] = pssvalues [j * msize + i] ;

                DFT (ckt->CKTpsspoints, ckt->CKTharms, &thd, psstimes, pssValues, ckt->CKTguessedFreq,
                         pssfreqs, pssmags, pssphases, pssnmags, pssnphases) ;

                for (j = 0 ; j < ckt->CKTharms ; j++)
                    pssResults [j * msize + i] = pssmags [j] ;
            }

            for (j = 0 ; j < ckt->CKTharms ; j++)
            {
                for (i = 0 ; i < msize ; i++)
                    ckt->CKTrhsOld [i + 1] = pssResults [j * msize + i] ;

                CKTdump (ckt, pssfreqs [j], job->PSSplot_fd) ;
            }
            /* ****************** */
            /* Ending DFT on data */
            /* ****************** */

            /* Terminates plot in Frequency Domain and frees the allocated memory */
            SPfrontEnd->OUTendPlot (job->PSSplot_fd) ;



            /* Francesco Lannutti's MOD */

            /* Verify the frequency found */
            max_freq = pssResults [msize] ;             /* max_freq = pssResults [1 * msize + 0] ; */
            position = 1 ;
            for (j = 1 ; j < ckt->CKTharms ; j++)
            {
                for (i = 0 ; i < msize ; i++)
                {
                    if (max_freq < pssResults [j * msize + i])
                    {
                        max_freq = pssResults [j * msize + i] ;
                        position = j ;
                    }
                }
            }

            if (pssfreqs [position] != ckt->CKTguessedFreq)
            {
                ckt->CKTguessedFreq = pssfreqs [position] ;
                fprintf (stderr, "The predicted fundamental frequency is incorrect.\nRelaunching the analysis...\n\n") ;
                fprintf (stderr, "The new guessed fundamental frequency is: %.6g\n\n", ckt->CKTguessedFreq) ;
                DCpss (ckt, 1) ;
            }
            /****************************/



            FREE (RHS_copy_se) ;
            FREE (RHS_copy_der) ;
            FREE (RHS_max) ;
            FREE (RHS_min) ;
//            FREE (S_old) ;
//            FREE (S_diff) ;
            FREE (pssfreqs) ;
            FREE (psstimes) ;
            FREE (pssValues) ;
            FREE (pssResults) ;
            FREE (pssmags) ;
            FREE (pssphases) ;
            FREE (pssnmags) ;
            FREE (pssnphases) ;
            FREE (appMatrix) ;
            FREE (oldMatrix) ;
//            FREE (mulMatrix) ;
            return (OK) ;
        }
    }
    /* ********************************** */
    /* **** END SHOOTING CODE BLOCK ***** */
    /* ********************************** */

    if (SPfrontEnd->IFpauseTest())
    {
        /* user requested pause... */
        UPDATE_STATS (DOING_TRAN) ;
        return (E_PAUSE) ;
    }

    /* RESUME */
resume:
#ifdef STEPDEBUG
    if ((ckt->CKTdelta <= ckt->CKTfinalTime / 50) && (ckt->CKTdelta <= ckt->CKTmaxStep))
    {
	;
    } else {
        if (ckt->CKTfinalTime / 50 < ckt->CKTmaxStep)
	    fprintf (stderr, "limited by Tstop/50\n") ;
	else
	    fprintf (stderr, "limited by Tmax == %g\n", ckt->CKTmaxStep) ;
    }
#endif

#ifdef HAS_WINDOWS
    if (ckt->CKTtime == 0.)
        SetAnalyse ("tran init", 0) ;
    else if ((!in_pss) && (shooting_cycle_counter > 0))
        SetAnalyse ("shooting", shooting_cycle_counter) ;
    else
        SetAnalyse ("tran", (int)((ckt->CKTtime * 1000.) / ckt->CKTfinalTime)) ;
#endif

    ckt->CKTdelta = MIN (ckt->CKTdelta, ckt->CKTmaxStep) ;

#ifdef XSPICE
/* gtri - begin - wbk - Cut integration order if first timepoint after breakpoint */
    /* if(ckt->CKTtime == g_mif_info.breakpoint.last) */
    if (AlmostEqualUlps (ckt->CKTtime, g_mif_info.breakpoint.last, 100))
        ckt->CKTorder = 1 ;
/* gtri - end   - wbk - Cut integration order if first timepoint after breakpoint */
#endif

    /* are we at a breakpoint, or indistinguishably close? */
    /* if ((ckt->CKTtime == ckt->CKTbreaks [0]) || (ckt->CKTbreaks [0] - */
    if (ckt->CKTbreaks [0] - ckt->CKTtime <= ckt->CKTdelmin)
    {
        /*if ( AlmostEqualUlps( ckt->CKTtime, ckt->CKTbreaks[0], 100 ) || (ckt->CKTbreaks[0] -
        *    (ckt->CKTtime) <= ckt->CKTdelmin)) {*/
        /* first timepoint after a breakpoint - cut integration order */
        /* and limit timestep to .1 times minimum of time to next breakpoint,
        *  and previous timestep
        */
        ckt->CKTorder = 1 ;

#ifdef STEPDEBUG
        if ((ckt->CKTdelta > .1 * ckt->CKTsaveDelta) || (ckt->CKTdelta > .1 * (ckt->CKTbreaks [1] - ckt->CKTbreaks [0])))
        {
            if (ckt->CKTsaveDelta < (ckt->CKTbreaks [1] - ckt->CKTbreaks [0]))
            {
                fprintf (stderr, "limited by pre-breakpoint delta (saveDelta: %1.10g, nxt_breakpt: %1.10g, curr_breakpt: %1.10g and CKTtime: %1.10g\n",
                         ckt->CKTsaveDelta, ckt->CKTbreaks [1], ckt->CKTbreaks [0], ckt->CKTtime) ;
            } else {
                fprintf (stderr, "limited by next breakpoint\n") ;
                fprintf (stderr, "(saveDelta: %1.10g, Delta: %1.10g, CKTtime: %1.10g and delmin: %1.10g\n",
                         ckt->CKTsaveDelta, ckt->CKTdelta, ckt->CKTtime, ckt->CKTdelmin) ;
	    }
	}
#endif

        if (ckt->CKTbreaks [1] - ckt->CKTbreaks [0] == 0)
            ckt->CKTdelta = ckt->CKTdelmin ;
        else
            ckt->CKTdelta = MIN (ckt->CKTdelta, .1 * MIN (ckt->CKTsaveDelta, ckt->CKTbreaks [1] - ckt->CKTbreaks [0])) ;

        if (firsttime)
        {
            ckt->CKTdelta /= 10 ;

#ifdef STEPDEBUG
            fprintf (stderr, "delta cut for initial timepoint\n") ;
#endif

        }

#ifdef XSPICE
    }

/* gtri - begin - wbk - Add Breakpoint stuff */
    if (ckt->CKTtime + ckt->CKTdelta >= g_mif_info.breakpoint.current)
    {
        /* If next time > temporary breakpoint, force it to the breakpoint */
        /* And mark that timestep was set by temporary breakpoint */
        ckt->CKTsaveDelta = ckt->CKTdelta ;
        ckt->CKTdelta = g_mif_info.breakpoint.current - ckt->CKTtime ;
        g_mif_info.breakpoint.last = ckt->CKTtime + ckt->CKTdelta ;
    } else {
        /* Else, mark that timestep was not set by temporary breakpoint */
        g_mif_info.breakpoint.last = ERR ;
    }
/* gtri - end - wbk - Add Breakpoint stuff */

/* gtri - begin - wbk - Modify Breakpoint stuff */
    /* Throw out any permanent breakpoint times <= current time */
    for (;;)
    {

#ifdef STEPDEBUG
        fprintf (stderr, "    brk_pt: %g    ckt_time: %g    ckt_min_break: %g\n", ckt->CKTbreaks [0], ckt->CKTtime, ckt->CKTminBreak) ;
#endif

        if (AlmostEqualUlps (ckt->CKTbreaks [0], ckt->CKTtime, 100) || ckt->CKTbreaks [0] <= ckt->CKTtime + ckt->CKTminBreak)
        {
            fprintf (stderr, "throwing out permanent breakpoint times <= current time (brk pt: %g)\n", ckt->CKTbreaks [0]) ;
            fprintf (stderr, "ckt_time: %g    ckt_min_break: %g\n", ckt->CKTtime, ckt->CKTminBreak) ;
	    CKTclrBreak (ckt) ;
        } else {
            break ;
        }
    }

    /* Force the breakpoint if appropriate */
    if (ckt->CKTtime + ckt->CKTdelta > ckt->CKTbreaks [0])
    {
        ckt->CKTbreak = 1 ;
        ckt->CKTsaveDelta = ckt->CKTdelta ;
        ckt->CKTdelta = ckt->CKTbreaks [0] - ckt->CKTtime ;
    }
/* gtri - end - wbk - Modify Breakpoint stuff */

#else /* !XSPICE */

        /* We don't want to get below delmin for no reason */
        ckt->CKTdelta = MAX (ckt->CKTdelta, ckt->CKTdelmin * 2.0) ;
    }
    else if (ckt->CKTtime + ckt->CKTdelta >= ckt->CKTbreaks [0])
    {
        ckt->CKTsaveDelta = ckt->CKTdelta ;
        ckt->CKTdelta = ckt->CKTbreaks [0] - ckt->CKTtime ;
        /* fprintf (stderr, "delta cut to %g to hit breakpoint\n" ,ckt->CKTdelta) ; */
        fflush (stdout) ;
        ckt->CKTbreak = 1 ; /* why? the current pt. is not a bkpt. */
    }

#ifdef CLUSTER
    if (!CLUsync (ckt->CKTtime, &ckt->CKTdelta, 0))
    {
        fprintf (stderr, "Sync error!\n") ;
        exit (0) ;
    }
#endif

#endif /* XSPICE */

#ifdef XSPICE
/* gtri - begin - wbk - Do event solution */
    if (ckt->evt->counts.num_insts > 0)
    {
        /* if time = 0 and op_alternate was specified as false during */
        /* dcop analysis, call any changed instances to let them */
        /* post their outputs with their associated delays */
        if ((ckt->CKTtime == 0.0) && (!ckt->evt->options.op_alternate))
            EVTiter (ckt) ;

        /* while there are events on the queue with event time <= next */
        /* projected analog time, process them */
        while ((g_mif_info.circuit.evt_step = EVTnext_time (ckt)) <= (ckt->CKTtime + ckt->CKTdelta))
        {
            /* Initialize temp analog bkpt to infinity */
            g_mif_info.breakpoint.current = ERR ;

            /* Pull items off queue and process them */
            EVTdequeue (ckt, g_mif_info.circuit.evt_step) ;
            EVTiter (ckt) ;

            /* If any instances have forced an earlier */
            /* next analog time, cut the delta */
            if (ckt->CKTbreaks [0] < g_mif_info.breakpoint.current)
                if (ckt->CKTbreaks [0] > ckt->CKTtime + ckt->CKTminBreak)
                    g_mif_info.breakpoint.current = ckt->CKTbreaks [0] ;

            if (g_mif_info.breakpoint.current < ckt->CKTtime + ckt->CKTdelta)
            {
                /* Breakpoint must be > last accepted timepoint and >= current event time */
                if (g_mif_info.breakpoint.current > ckt->CKTtime + ckt->CKTminBreak  && g_mif_info.breakpoint.current >= g_mif_info.circuit.evt_step)
                {
                    ckt->CKTsaveDelta = ckt->CKTdelta ;
                    ckt->CKTdelta = g_mif_info.breakpoint.current - ckt->CKTtime ;
                    g_mif_info.breakpoint.last = ckt->CKTtime + ckt->CKTdelta ;
                }
            }
        } /* end while next event time <= next analog time */
    } /* end if there are event instances */
/* gtri - end - wbk - Do event solution */
#endif

    /* What is that??? */
    for (i = 5 ; i >= 0 ; i--)
        ckt->CKTdeltaOld [i + 1] = ckt->CKTdeltaOld [i] ;

    ckt->CKTdeltaOld [0] = ckt->CKTdelta ;
    {
        double *temp = ckt->CKTstates [ckt->CKTmaxOrder + 1] ;

        for (i = ckt->CKTmaxOrder ; i >= 0 ; i--)
            ckt->CKTstates [i + 1] = ckt->CKTstates [i] ;

        ckt->CKTstates [0] = temp ;
    }

    /* 600 */
    for (;;)
    {

#ifdef CLUSTER
        redostep = 1 ;
#endif

#ifdef XSPICE
/* gtri - add - wbk - 4/17/91 - Fix Berkeley bug */
        /* This is needed here to allow CAPask output currents */
        /* during Transient analysis.  A grep for CKTcurrentAnalysis */
        /* indicates that it should not hurt anything else ... */

        ckt->CKTcurrentAnalysis = DOING_TRAN ;
/* gtri - end - wbk - 4/17/91 - Fix Berkeley bug */
#endif

        olddelta = ckt->CKTdelta ;
        /* time abort? */


        /* ************************************ */
        /* ********** CKTtime update ********** */
        /* ************************************ */

        /* Force the tran analysis to evaluate requested breakpoints. Breakpoints are even more closer as
           the next occurence of guessed period is approaching. La lunga notte dei robot viventi... */
        /* If it's in Shooting */
        if ((!in_stabilization) && (!in_pss))
        {
            if ((ckt->CKTtime - time_temp + 1 / ckt->CKTguessedFreq > (1 / ckt->CKTguessedFreq) * 0.5))
            {
                if ((ckt->CKTtime - time_temp + 1 / ckt->CKTguessedFreq > (1 / ckt->CKTguessedFreq) * 0.8))
                {
                    if ((ckt->CKTtime - time_temp + 1 / ckt->CKTguessedFreq > (1 / ckt->CKTguessedFreq) * 0.995))
                    {
                        CKTsetBreak (ckt, ckt->CKTtime + (1 / ckt->CKTguessedFreq) / 5.0e3) ;
                    } else {
                        CKTsetBreak (ckt, ckt->CKTtime + (1 / ckt->CKTguessedFreq) / 5.0e2) ;
                    }
                } else {
                    CKTsetBreak (ckt, ckt->CKTtime + (1 / ckt->CKTguessedFreq) / 1.0e2) ;
                }
            }
        }

        /* In early PSS implementation I used to take fixed delta when circuit had reached PSS.
        This choice eventually caused the algorithm hang if a non convergence of Newton-Rhapson
        was found. The following lines are kept here as a trace of past errors...*/
        else if ((in_pss) && (!in_stabilization)) 
        {

#ifdef STEPDEBUG
            fprintf (stderr, "Frequency: %g\n", ckt->CKTguessedFreq) ;
#endif

        }
        else if ((in_pss) && (in_stabilization))
        {
            fprintf (stderr, "PSS algorithm cannot be IN_PSS and IN_STABILIZATION at the same time. Analysis aborted!\n");
            return (E_PANIC) ;
        }
        /* ************************************ */
        /* ******* END CKTtime update ********* */
        /* ************************************ */

	ckt->CKTtime += ckt->CKTdelta ;

#ifdef CLUSTER
        CLUinput (ckt) ;
#endif

        ckt->CKTdeltaOld [0] = ckt->CKTdelta ;
        NIcomCof (ckt) ;

#ifdef PREDICTOR
       error = NIpred (ckt) ;
#endif /* PREDICTOR */

        save_mode = ckt->CKTmode ;
        save_order = ckt->CKTorder ;

#ifdef XSPICE
/* gtri - begin - wbk - Add Breakpoint stuff */
        /* Initialize temporary breakpoint to infinity */
        g_mif_info.breakpoint.current = ERR ;
/* gtri - end - wbk - Add Breakpoint stuff */

/* gtri - begin - wbk - add convergence problem reporting flags */
        /* delta is forced to equal delmin on last attempt near line 650 */
        if (ckt->CKTdelta <= ckt->CKTdelmin)
            ckt->enh->conv_debug.last_NIiter_call = MIF_TRUE ;
        else
            ckt->enh->conv_debug.last_NIiter_call = MIF_FALSE ;

/* gtri - begin - wbk - add convergence problem reporting flags */
/* gtri - begin - wbk - Call all hybrids */
/* gtri - begin - wbk - Set evt_step */
        if (ckt->evt->counts.num_insts > 0)
            g_mif_info.circuit.evt_step = ckt->CKTtime ;
/* gtri - end - wbk - Set evt_step */
#endif

        converged = NIiter (ckt, ckt->CKTtranMaxIter) ;

#ifdef XSPICE
        if (ckt->evt->counts.num_insts > 0)
        {
            g_mif_info.circuit.evt_step = ckt->CKTtime ;
            EVTcall_hybrids (ckt) ;
        }
/* gtri - end - wbk - Call all hybrids */
#endif

        ckt->CKTstat->STATtimePts++ ;
        ckt->CKTmode = (ckt->CKTmode & MODEUIC) | MODETRAN | MODEINITPRED ;
        if (firsttime)
        {
            for (i = 0 ; i < ckt->CKTnumStates ; i++)
            {
                ckt->CKTstate2 [i] = ckt->CKTstate1 [i] ;
                ckt->CKTstate3 [i] = ckt->CKTstate1 [i] ;
            }
        }

        /* txl, cpl addition */
        if (converged == 1111)
	    return (converged) ;

#ifdef STEPDEBUG
        if (in_pss)
            fprintf (stderr, "in_stabilization: %d, in_pss: %d, converged: %d\n", in_stabilization, in_pss, converged) ;
#endif

        if (converged != 0)
        {

#ifndef CLUSTER
            ckt->CKTtime = ckt->CKTtime -ckt->CKTdelta ;
            ckt->CKTstat->STATrejected++ ;
#endif

            ckt->CKTdelta = ckt->CKTdelta / 8 ;

#ifdef STEPDEBUG
            fprintf (stderr, "delta cut to %g for non-convergance\n", ckt->CKTdelta) ;
            fflush (stdout) ;
#endif

            if (firsttime)
                ckt->CKTmode = (ckt->CKTmode & MODEUIC) | MODETRAN | MODEINITTRAN ;

            ckt->CKTorder = 1 ;

#ifdef XSPICE
/* gtri - begin - wbk - Add Breakpoint stuff */

        /* Force backup if temporary breakpoint is < current time */
        }
        else if (g_mif_info.breakpoint.current < ckt->CKTtime)
        {
            ckt->CKTsaveDelta = ckt->CKTdelta ;
            ckt->CKTtime -= ckt->CKTdelta ;
            ckt->CKTdelta = g_mif_info.breakpoint.current - ckt->CKTtime ;
            g_mif_info.breakpoint.last = ckt->CKTtime + ckt->CKTdelta ;

            if (firsttime)
                ckt->CKTmode = (ckt->CKTmode & MODEUIC)|MODETRAN | MODEINITTRAN ;

            ckt->CKTorder = 1 ;

/* gtri - end - wbk - Add Breakpoint stuff */
#endif

        } else {
            if (firsttime)
            {
                firsttime = 0 ;

#ifndef CLUSTER
                /* no check on first time point */
                goto nextTime ;
#else
                redostep = 0 ;
                goto chkStep ;
#endif

            }

            newdelta = ckt->CKTdelta ;
            error = CKTtrunc (ckt, &newdelta) ;
            if (error)
            {
                UPDATE_STATS (DOING_TRAN) ;
                return (error) ;
            }

            if (newdelta > .9 * ckt->CKTdelta)
            {
                if (ckt->CKTorder == 1)
                {
                    newdelta = ckt->CKTdelta ;
                    ckt->CKTorder = 2 ;
                    error = CKTtrunc (ckt, &newdelta) ;
                    if (error)
                    {
                        UPDATE_STATS (DOING_TRAN) ;
                        return (error) ;
                    }

                    if (newdelta <= 1.05 * ckt->CKTdelta)
                        ckt->CKTorder = 1 ;
                }

                /* time point OK - 630 */
                ckt->CKTdelta = newdelta ;

#ifdef NDEV
                /* Show a time process indicator, by Gong Ding, gdiso@ustc.edu */
                if (ckt->CKTtime / ckt->CKTfinalTime * 100 < 10.0)
                    fprintf (stderr, "%%%3.2lf\b\b\b\b\b", ckt->CKTtime / ckt->CKTfinalTime * 100) ;
                else if (ckt->CKTtime / ckt->CKTfinalTime * 100 < 100.0)
                    fprintf (stderr, "%%%4.2lf\b\b\b\b\b\b", ckt->CKTtime / ckt->CKTfinalTime * 100) ;
                else
                    fprintf (stderr, "%%%5.2lf\b\b\b\b\b\b\b", ckt->CKTtime / ckt->CKTfinalTime * 100) ;

                fflush (stdout) ;
#endif

#ifdef STEPDEBUG
                fprintf (stderr, "delta set to truncation error result: %g. Point accepted at CKTtime: %g\n", ckt->CKTdelta, ckt->CKTtime) ;
                fflush (stdout) ;
#endif

#ifndef CLUSTER
                /* go to 650 - trapezoidal */
                goto nextTime ;
#else
                redostep = 0 ;
                goto chkStep ;
#endif

            } else { /* newdelta <= .9 * ckt->CKTdelta */

#ifndef CLUSTER
                ckt->CKTtime = ckt->CKTtime - ckt->CKTdelta ;
                ckt->CKTstat->STATrejected++ ;
#endif

                ckt->CKTdelta = newdelta ;

#ifdef STEPDEBUG
                fprintf (stderr, "delta set to truncation error result:point rejected\n") ;
#endif

            }
        }

        if (ckt->CKTdelta <= ckt->CKTdelmin)
        {
            if (olddelta > ckt->CKTdelmin)
            {
                ckt->CKTdelta = ckt->CKTdelmin;

#ifdef STEPDEBUG
                fprintf (stderr, "delta at delmin\n");
#endif

            } else {
                UPDATE_STATS (DOING_TRAN);
                errMsg = CKTtrouble (ckt, "Timestep too small");
                return (E_TIMESTEP);
            }
        }

#ifdef XSPICE
/* gtri - begin - wbk - Do event backup */
        if (ckt->evt->counts.num_insts > 0)
            EVTbackup (ckt, ckt->CKTtime + ckt->CKTdelta);
/* gtri - end - wbk - Do event backup */
#endif

#ifdef CLUSTER
chkStep:
        if (CLUsync (ckt->CKTtime, &ckt->CKTdelta, redostep)) {
            goto nextTime;
        } else {
            ckt->CKTtime -= olddelta;
            ckt->CKTstat->STATrejected++;
        }
#endif

    } /* END OF FOR (;;) */
} /* NOTREACHED */

static int
DFT
(
    long int ndata,  /* number of entries in the Time and Value arrays */
    int numFreq,     /* number of harmonics to calculate */
    double *thd,     /* total harmonic distortion (percent) to be returned */
    double *Time,    /* times at which the voltage/current values were measured */
    double *Value,   /* voltage or current vector whose transform is desired */
    double FundFreq, /* the fundamental frequency of the analysis */
    double *Freq,    /* the frequency value of the various harmonics */
    double *Mag,     /* the Magnitude of the fourier transform */
    double *Phase,   /* the Phase of the fourier transform */
    double *nMag,    /* the normalized magnitude of the transform: nMag (fund) = 1 */
    double *nPhase   /* the normalized phase of the transform: Nphase (fund) = 0 */
)
{
    /* Note: we can consider these as a set of arrays.  The sizes are:
     * Time [ndata], Value [ndata], Freq [numFreq], Mag [numfreq],
     * Phase [numfreq], nMag [numfreq], nPhase [numfreq]
     *
     * The arrays must all be allocated by the caller.
     * The Time and Value array must be reasonably distributed over at
     * least one full period of the fundamental Frequency for the
     * fourier transform to be useful.  The function will take the
     * last period of the frequency as data for the transform.
     *
     * We are assuming that the caller has provided exactly one period
     * of the fundamental frequency.  */
    int i, j;
    double tmp;

    NG_IGNORE (Time);

    /* clear output/computation arrays */

    for (i = 0; i < numFreq; i++) {
        Mag [i] = 0;
        Phase [i] = 0;
    }

    for (i = 0; i < ndata; i++) {
        for (j = 0; j < numFreq; j++) {
            Mag [j] += (Value [i] * sin (j * 2.0 * M_PI * i / ((double)ndata)));
            Phase [j] += (Value [i] * cos (j * 2.0 * M_PI * i / ((double)ndata)));
        }
    }

    Mag [0] = Phase [0] / (double)ndata;
    Phase [0] = 0;
    nMag [0] = 0;
    nPhase [0] = 0;
    Freq [0] = 0;
    *thd = 0;

    for (i = 1; i < numFreq; i++) {
        tmp = Mag [i] * 2.0 / (double)ndata;
        Phase [i] *= 2.0 / (double)ndata;
        Freq [i] = i * FundFreq;
        Mag [i] = sqrt (tmp * tmp + Phase [i] * Phase [i]);
        Phase [i] = atan2 (Phase [i], tmp) * 180.0 / M_PI;
        nMag [i] = Mag [i] / Mag [1];
        nPhase [i] = Phase [i] - Phase [1];
        if (i > 1)
            *thd += nMag [i] * nMag [i];
    }

    *thd = 100 * sqrt (*thd);
    return (OK);
}
