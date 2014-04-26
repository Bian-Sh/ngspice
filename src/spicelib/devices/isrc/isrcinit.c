#include "ngspice/config.h"

#include "ngspice/devdefs.h"

#include "isrcitf.h"
#include "isrcext.h"
#include "isrcinit.h"

#ifdef USE_CUSPICE
#include "ngspice/CUSPICE/CUSPICE.h"
#endif

SPICEdev ISRCinfo = {
    {
        "Isource",  
        "Independent current source",

        &ISRCnSize,
        &ISRCnSize,
        ISRCnames,

        &ISRCpTSize,
        ISRCpTable,

        NULL,
        NULL,

#ifdef XSPICE
/*----  Fixed by SDB 5.2.2003 to enable XSPICE/tclspice integration  -----*/
        NULL,  /* This is a SPICE device, it has no MIF info data */

        0,     /* This is a SPICE device, it has no MIF info data */
        NULL,  /* This is a SPICE device, it has no MIF info data */

        0,     /* This is a SPICE device, it has no MIF info data */
        NULL,  /* This is a SPICE device, it has no MIF info data */

        0,     /* This is a SPICE device, it has no MIF info data */
        NULL,  /* This is a SPICE device, it has no MIF info data */
/*---------------------------  End of SDB fix   -------------------------*/
#endif

	DEV_DEFAULT
    },

 /* DEVparam      */ ISRCparam,
 /* DEVmodParam   */ NULL,
#ifdef USE_CUSPICE
 /* DEVload       */ cuISRCload,
#else
 /* DEVload       */ ISRCload,
#endif
#ifdef USE_CUSPICE
 /* DEVsetup      */ ISRCsetup,
#else
 /* DEVsetup      */ NULL,
#endif
 /* DEVunsetup    */ NULL,
 /* DEVpzSetup    */ NULL,
 /* DEVtemperature*/ ISRCtemp,
 /* DEVtrunc      */ NULL,
 /* DEVfindBranch */ NULL,
 /* DEVacLoad     */ ISRCacLoad,
 /* DEVaccept     */ ISRCaccept,
 /* DEVdestroy    */ ISRCdestroy,
 /* DEVmodDelete  */ ISRCmDelete,
 /* DEVdelete     */ ISRCdelete,
 /* DEVsetic      */ NULL,
 /* DEVask        */ ISRCask,
 /* DEVmodAsk     */ NULL,
 /* DEVpzLoad     */ NULL,
 /* DEVconvTest   */ NULL,
 /* DEVsenSetup   */ NULL,
 /* DEVsenLoad    */ NULL,
 /* DEVsenUpdate  */ NULL,
 /* DEVsenAcLoad  */ NULL,
 /* DEVsenPrint   */ NULL,
 /* DEVsenTrunc   */ NULL,
 /* DEVdisto      */ NULL,	/* DISTO */
 /* DEVnoise      */ NULL,	/* NOISE */
 /* DEVsoaCheck   */ NULL,
#ifdef CIDER
 /* DEVdump       */ NULL,
 /* DEVacct       */ NULL,
#endif                        
 /* DEVinstSize   */ &ISRCiSize,
 /* DEVmodSize    */ &ISRCmSize,

#ifdef KLU
 /* DEVbindCSC        */   NULL,
 /* DEVbindCSCComplex */   NULL,
 /* DEVbindCSCComplexToReal */  NULL,
#endif

#ifdef USE_CUSPICE
 /* cuDEVdestroy */ cuISRCdestroy,
 /* DEVtopology */  ISRCtopology,
#endif

};


SPICEdev *
get_isrc_info(void)
{
    return &ISRCinfo;
}
