#ifndef SMP
#define SMP

/* Typedef removed by Francesco Lannutti (2012-02) to create the new SMPmatrix structure */
/*
typedef  struct MatrixFrame     SMPmatrix;
*/
typedef  struct MatrixFrame     MatrixFrame;
typedef  struct MatrixElement  *SMPelement;

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified: 2000  AlansFixes
**********/

#include <stdio.h>
#include <math.h>
#include "ngspice/complex.h"

#ifdef KLU
#include "ngspice/klu.h"
#endif

struct SMPmatrix {
    MatrixFrame *SPmatrix ;               /* pointer to sparse matrix */

#ifdef KLU
    klu_common *CKTkluCommon ;            /* KLU common object */
    klu_symbolic *CKTkluSymbolic ;        /* KLU symbolic object */
    klu_numeric *CKTkluNumeric ;          /* KLU numeric object */
    int *CKTkluAp ;                       /* KLU column pointer */
    int *CKTkluAi ;                       /* KLU row pointer */
    double *CKTkluAx ;                    /* KLU element */
    double *CKTkluIntermediate ;          /* KLU RHS Intermediate for Solve Real Step */
    double *CKTkluIntermediate_Complex ;  /* KLU iRHS Intermediate for Solve Complex Step */
    double **CKTbind_Sparse ;             /* KLU - Sparse original element position */
    double **CKTbind_CSC ;                /* KLU - KLU new element position */
    double **CKTbind_CSC_Complex ;        /* KLU - KLU new element position in Complex analysis */
    double **CKTdiag_CSC ;                 /* KLU pointer to diagonal element to perform Gmin */
    int CKTkluN ;                         /* KLU N, copied */
    int CKTklunz ;                        /* KLU nz, copied for AC Analysis */
    int CKTkluMODE ;                      /* KLU MODE parameter to enable KLU or not from the heuristic */
    #define CKTkluON  1                   /* KLU MODE ON  definition */
    #define CKTkluOFF 0                   /* KLU MODE OFF definition */
#endif

};

/* SMPmatrix structure alias - Francesco Lannutti (2012-02) */
typedef struct SMPmatrix SMPmatrix ;


#ifdef KLU
void SMPmatrix_CSC ( SMPmatrix * ) ;
void SMPnnz ( SMPmatrix * ) ;
#endif
int SMPaddElt( SMPmatrix *, int , int , double );
double * SMPmakeElt( SMPmatrix * , int , int );
void SMPcClear( SMPmatrix *);
void SMPclear( SMPmatrix *);
int SMPcLUfac( SMPmatrix *, double );
int SMPluFac( SMPmatrix *, double , double );
int SMPcReorder( SMPmatrix * , double , double , int *);
int SMPreorder( SMPmatrix * , double , double , double );
void SMPcaSolve(SMPmatrix *Matrix, double RHS[], double iRHS[],
		double Spare[], double iSpare[]);
void SMPcSolve( SMPmatrix *, double [], double [], double [], double []);
void SMPsolve( SMPmatrix *, double [], double []);
int SMPmatSize( SMPmatrix *);
int SMPnewMatrix( SMPmatrix * );
void SMPdestroy( SMPmatrix *);
int SMPpreOrder( SMPmatrix *);
void SMPprint( SMPmatrix * , FILE *);
void SMPgetError( SMPmatrix *, int *, int *);
int SMPcProdDiag( SMPmatrix *, SPcomplex *, int *);
int SMPcDProd(SMPmatrix *Matrix, SPcomplex *pMantissa, int *pExponent);
SMPelement * SMPfindElt( SMPmatrix *, int , int , int );
int SMPcZeroCol(SMPmatrix *eMatrix, int Col);
int SMPcAddCol(SMPmatrix *eMatrix, int Accum_Col, int Addend_Col);
int SMPzeroRow(SMPmatrix *eMatrix, int Row);

/* Correction for the Spertica's hack */
extern void SMPgmo ( SMPmatrix *, int, double * ) ;
/* End of Correction for the Spertica's hack */

#ifdef PARALLEL_ARCH
void SMPcombine(SMPmatrix *Matrix, double RHS[], double Spare[]);
void SMPcCombine(SMPmatrix *Matrix, double RHS[], double Spare[],
		 double iRHS[], double iSpare[]);
#endif

#endif /*SMP*/
