/* $Header$ */
/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2017
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 * 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */

#include <iostream>
#include <iomanip>

#include <vector>

#include "ac/f2c.h"
#include "ac/arpack.h"

#include "solman.h"
#include "naivewrap.h"

/* from dnaupd.f distributed with arpack:

c\Description: 
c  Reverse communication interface for the Implicitly Restarted Arnoldi
c  iteration. This subroutine computes approximations to a few eigenpairs 
c  of a linear operator "OP" with respect to a semi-inner product defined by 
c  a symmetric positive semi-definite real matrix B. B may be the identity 
c  matrix. NOTE: If the linear operator "OP" is real and symmetric 
c  with respect to the real positive semi-definite symmetric matrix B, 
c  i.e. B*OP = (OP`)*B, then subroutine dsaupd  should be used instead.
c
c  The computed approximate eigenvalues are called Ritz values and
c  the corresponding approximate eigenvectors are called Ritz vectors.
c
c  dnaupd  is usually called iteratively to solve one of the 
c  following problems:
c
c  Mode 1:  A*x = lambda*x.
c           ===> OP = A  and  B = I.
c
c  Mode 2:  A*x = lambda*M*x, M symmetric positive definite
c           ===> OP = inv[M]*A  and  B = M.
c           ===> (If M can be factored see remark 3 below)
c
c  Mode 3:  A*x = lambda*M*x, M symmetric semi-definite
c           ===> OP = Real_Part{ inv[A - sigma*M]*M }  and  B = M. 
c           ===> shift-and-invert mode (in real arithmetic)
c           If OP*x = amu*x, then 
c           amu = 1/2 * [ 1/(lambda-sigma) + 1/(lambda-conjg(sigma)) ].
c           Note: If sigma is real, i.e. imaginary part of sigma is zero;
c                 Real_Part{ inv[A - sigma*M]*M } == inv[A - sigma*M]*M 
c                 amu == 1/(lambda-sigma). 
c  
c  Mode 4:  A*x = lambda*M*x, M symmetric semi-definite
c           ===> OP = Imaginary_Part{ inv[A - sigma*M]*M }  and  B = M. 
c           ===> shift-and-invert mode (in real arithmetic)
c           If OP*x = amu*x, then 
c           amu = 1/2i * [ 1/(lambda-sigma) - 1/(lambda-conjg(sigma)) ].
c
c  Both mode 3 and 4 give the same enhancement to eigenvalues close to
c  the (complex) shift sigma.  However, as lambda goes to infinity,
c  the operator OP in mode 4 dampens the eigenvalues more strongly than
c  does OP defined in mode 3.
c
c  NOTE: The action of w <- inv[A - sigma*M]*v or w <- inv[M]*v
c        should be accomplished either by a direct method
c        using a sparse matrix factorization and solving
c
c           [A - sigma*M]*w = v  or M*w = v,
c
c        or through an iterative method for solving these
c        systems.  If an iterative method is used, the
c        convergence test must be more stringent than
c        the accuracy requirements for the eigenvalue
c        approximations.
c
c\Usage:
c  call dnaupd 
c     ( IDO, BMAT, N, WHICH, NEV, TOL, RESID, NCV, V, LDV, IPARAM,
c       IPNTR, WORKD, WORKL, LWORKL, INFO )
c
c\Arguments
c  IDO     Integer.  (INPUT/OUTPUT)
c          Reverse communication flag.  IDO must be zero on the first 
c          call to dnaupd .  IDO will be set internally to
c          indicate the type of operation to be performed.  Control is
c          then given back to the calling routine which has the
c          responsibility to carry out the requested operation and call
c          dnaupd  with the result.  The operand is given in
c          WORKD(IPNTR(1)), the result must be put in WORKD(IPNTR(2)).
c          -------------------------------------------------------------
c          IDO =  0: first call to the reverse communication interface
c          IDO = -1: compute  Y = OP * X  where
c                    IPNTR(1) is the pointer into WORKD for X,
c                    IPNTR(2) is the pointer into WORKD for Y.
c                    This is for the initialization phase to force the
c                    starting vector into the range of OP.
c          IDO =  1: compute  Y = OP * X  where
c                    IPNTR(1) is the pointer into WORKD for X,
c                    IPNTR(2) is the pointer into WORKD for Y.
c                    In mode 3 and 4, the vector B * X is already
c                    available in WORKD(ipntr(3)).  It does not
c                    need to be recomputed in forming OP * X.
c          IDO =  2: compute  Y = B * X  where
c                    IPNTR(1) is the pointer into WORKD for X,
c                    IPNTR(2) is the pointer into WORKD for Y.
c          IDO =  3: compute the IPARAM(8) real and imaginary parts 
c                    of the shifts where INPTR(14) is the pointer
c                    into WORKL for placing the shifts. See Remark
c                    5 below.
c          IDO = 99: done
c          -------------------------------------------------------------
c             
c  BMAT    Character*1.  (INPUT)
c          BMAT specifies the type of the matrix B that defines the
c          semi-inner product for the operator OP.
c          BMAT = 'I' -> standard eigenvalue problem A*x = lambda*x
c          BMAT = 'G' -> generalized eigenvalue problem A*x = lambda*B*x
c
c  N       Integer.  (INPUT)
c          Dimension of the eigenproblem.
c
c  WHICH   Character*2.  (INPUT)
c          'LM' -> want the NEV eigenvalues of largest magnitude.
c          'SM' -> want the NEV eigenvalues of smallest magnitude.
c          'LR' -> want the NEV eigenvalues of largest real part.
c          'SR' -> want the NEV eigenvalues of smallest real part.
c          'LI' -> want the NEV eigenvalues of largest imaginary part.
c          'SI' -> want the NEV eigenvalues of smallest imaginary part.
c
c  NEV     Integer.  (INPUT)
c          Number of eigenvalues of OP to be computed. 0 < NEV < N-1.
c
c  TOL     Double precision  scalar.  (INPUT)
c          Stopping criterion: the relative accuracy of the Ritz value 
c          is considered acceptable if BOUNDS(I) .LE. TOL*ABS(RITZ(I))
c          where ABS(RITZ(I)) is the magnitude when RITZ(I) is complex.
c          DEFAULT = DLAMCH ('EPS')  (machine precision as computed
c                    by the LAPACK auxiliary subroutine DLAMCH ).
c
c  RESID   Double precision  array of length N.  (INPUT/OUTPUT)
c          On INPUT: 
c          If INFO .EQ. 0, a random initial residual vector is used.
c          If INFO .NE. 0, RESID contains the initial residual vector,
c                          possibly from a previous run.
c          On OUTPUT:
c          RESID contains the final residual vector.
c
c  NCV     Integer.  (INPUT)
c          Number of columns of the matrix V. NCV must satisfy the two
c          inequalities 2 <= NCV-NEV and NCV <= N.
c          This will indicate how many Arnoldi vectors are generated 
c          at each iteration.  After the startup phase in which NEV 
c          Arnoldi vectors are generated, the algorithm generates 
c          approximately NCV-NEV Arnoldi vectors at each subsequent update 
c          iteration. Most of the cost in generating each Arnoldi vector is 
c          in the matrix-vector operation OP*x. 
c          NOTE: 2 <= NCV-NEV in order that complex conjugate pairs of Ritz 
c          values are kept together. (See remark 4 below)
c
c  V       Double precision  array N by NCV.  (OUTPUT)
c          Contains the final set of Arnoldi basis vectors. 
c
c  LDV     Integer.  (INPUT)
c          Leading dimension of V exactly as declared in the calling program.
c
c  IPARAM  Integer array of length 11.  (INPUT/OUTPUT)
c          IPARAM(1) = ISHIFT: method for selecting the implicit shifts.
c          The shifts selected at each iteration are used to restart
c          the Arnoldi iteration in an implicit fashion.
c          -------------------------------------------------------------
c          ISHIFT = 0: the shifts are provided by the user via
c                      reverse communication.  The real and imaginary
c                      parts of the NCV eigenvalues of the Hessenberg
c                      matrix H are returned in the part of the WORKL 
c                      array corresponding to RITZR and RITZI. See remark 
c                      5 below.
c          ISHIFT = 1: exact shifts with respect to the current
c                      Hessenberg matrix H.  This is equivalent to 
c                      restarting the iteration with a starting vector
c                      that is a linear combination of approximate Schur
c                      vectors associated with the "wanted" Ritz values.
c          -------------------------------------------------------------
c
c          IPARAM(2) = No longer referenced.
c
c          IPARAM(3) = MXITER
c          On INPUT:  maximum number of Arnoldi update iterations allowed. 
c          On OUTPUT: actual number of Arnoldi update iterations taken. 
c
c          IPARAM(4) = NB: blocksize to be used in the recurrence.
c          The code currently works only for NB = 1.
c
c          IPARAM(5) = NCONV: number of "converged" Ritz values.
c          This represents the number of Ritz values that satisfy
c          the convergence criterion.
c
c          IPARAM(6) = IUPD
c          No longer referenced. Implicit restarting is ALWAYS used.  
c
c          IPARAM(7) = MODE
c          On INPUT determines what type of eigenproblem is being solved.
c          Must be 1,2,3,4; See under \Description of dnaupd  for the 
c          four modes available.
c
c          IPARAM(8) = NP
c          When ido = 3 and the user provides shifts through reverse
c          communication (IPARAM(1)=0), dnaupd  returns NP, the number
c          of shifts the user is to provide. 0 < NP <=NCV-NEV. See Remark
c          5 below.
c
c          IPARAM(9) = NUMOP, IPARAM(10) = NUMOPB, IPARAM(11) = NUMREO,
c          OUTPUT: NUMOP  = total number of OP*x operations,
c                  NUMOPB = total number of B*x operations if BMAT='G',
c                  NUMREO = total number of steps of re-orthogonalization.        
c
c  IPNTR   Integer array of length 14.  (OUTPUT)
c          Pointer to mark the starting locations in the WORKD and WORKL
c          arrays for matrices/vectors used by the Arnoldi iteration.
c          -------------------------------------------------------------
c          IPNTR(1): pointer to the current operand vector X in WORKD.
c          IPNTR(2): pointer to the current result vector Y in WORKD.
c          IPNTR(3): pointer to the vector B * X in WORKD when used in 
c                    the shift-and-invert mode.
c          IPNTR(4): pointer to the next available location in WORKL
c                    that is untouched by the program.
c          IPNTR(5): pointer to the NCV by NCV upper Hessenberg matrix
c                    H in WORKL.
c          IPNTR(6): pointer to the real part of the ritz value array 
c                    RITZR in WORKL.
c          IPNTR(7): pointer to the imaginary part of the ritz value array
c                    RITZI in WORKL.
c          IPNTR(8): pointer to the Ritz estimates in array WORKL associated
c                    with the Ritz values located in RITZR and RITZI in WORKL.
c
c          IPNTR(14): pointer to the NP shifts in WORKL. See Remark 5 below.
c
c          Note: IPNTR(9:13) is only referenced by dneupd . See Remark 2 below.
c
c          IPNTR(9):  pointer to the real part of the NCV RITZ values of the 
c                     original system.
c          IPNTR(10): pointer to the imaginary part of the NCV RITZ values of 
c                     the original system.
c          IPNTR(11): pointer to the NCV corresponding error bounds.
c          IPNTR(12): pointer to the NCV by NCV upper quasi-triangular
c                     Schur matrix for H.
c          IPNTR(13): pointer to the NCV by NCV matrix of eigenvectors
c                     of the upper Hessenberg matrix H. Only referenced by
c                     dneupd  if RVEC = .TRUE. See Remark 2 below.
c          -------------------------------------------------------------
c          
c  WORKD   Double precision  work array of length 3*N.  (REVERSE COMMUNICATION)
c          Distributed array to be used in the basic Arnoldi iteration
c          for reverse communication.  The user should not use WORKD 
c          as temporary workspace during the iteration. Upon termination
c          WORKD(1:N) contains B*RESID(1:N). If an invariant subspace
c          associated with the converged Ritz values is desired, see remark
c          2 below, subroutine dneupd  uses this output.
c          See Data Distribution Note below.  
c
c  WORKL   Double precision  work array of length LWORKL.  (OUTPUT/WORKSPACE)
c          Private (replicated) array on each PE or array allocated on
c          the front end.  See Data Distribution Note below.
c
c  LWORKL  Integer.  (INPUT)
c          LWORKL must be at least 3*NCV**2 + 6*NCV.
c
c  INFO    Integer.  (INPUT/OUTPUT)
c          If INFO .EQ. 0, a randomly initial residual vector is used.
c          If INFO .NE. 0, RESID contains the initial residual vector,
c                          possibly from a previous run.
c          Error flag on output.
c          =  0: Normal exit.
c          =  1: Maximum number of iterations taken.
c                All possible eigenvalues of OP has been found. IPARAM(5)  
c                returns the number of wanted converged Ritz values.
c          =  2: No longer an informational error. Deprecated starting
c                with release 2 of ARPACK.
c          =  3: No shifts could be applied during a cycle of the 
c                Implicitly restarted Arnoldi iteration. One possibility 
c                is to increase the size of NCV relative to NEV. 
c                See remark 4 below.
c          = -1: N must be positive.
c          = -2: NEV must be positive.
c          = -3: NCV-NEV >= 2 and less than or equal to N.
c          = -4: The maximum number of Arnoldi update iteration 
c                must be greater than zero.
c          = -5: WHICH must be one of 'LM', 'SM', 'LR', 'SR', 'LI', 'SI'
c          = -6: BMAT must be one of 'I' or 'G'.
c          = -7: Length of private work array is not sufficient.
c          = -8: Error return from LAPACK eigenvalue calculation;
c          = -9: Starting vector is zero.
c          = -10: IPARAM(7) must be 1,2,3,4.
c          = -11: IPARAM(7) = 1 and BMAT = 'G' are incompatable.
c          = -12: IPARAM(1) must be equal to 0 or 1.
c          = -9999: Could not build an Arnoldi factorization.
c                   IPARAM(5) returns the size of the current Arnoldi
c                   factorization.
c
c\Remarks
c  1. The computed Ritz values are approximate eigenvalues of OP. The
c     selection of WHICH should be made with this in mind when
c     Mode = 3 and 4.  After convergence, approximate eigenvalues of the
c     original problem may be obtained with the ARPACK subroutine dneupd .
c
c  2. If a basis for the invariant subspace corresponding to the converged Ritz 
c     values is needed, the user must call dneupd  immediately following 
c     completion of dnaupd . This is new starting with release 2 of ARPACK.
c
c  3. If M can be factored into a Cholesky factorization M = LL`
c     then Mode = 2 should not be selected.  Instead one should use
c     Mode = 1 with  OP = inv(L)*A*inv(L`).  Appropriate triangular 
c     linear systems should be solved with L and L` rather
c     than computing inverses.  After convergence, an approximate
c     eigenvector z of the original problem is recovered by solving
c     L`z = x  where x is a Ritz vector of OP.
c
c  4. At present there is no a-priori analysis to guide the selection
c     of NCV relative to NEV.  The only formal requrement is that NCV > NEV + 2.
c     However, it is recommended that NCV .ge. 2*NEV+1.  If many problems of
c     the same type are to be solved, one should experiment with increasing
c     NCV while keeping NEV fixed for a given test problem.  This will 
c     usually decrease the required number of OP*x operations but it
c     also increases the work and storage required to maintain the orthogonal
c     basis vectors.  The optimal "cross-over" with respect to CPU time
c     is problem dependent and must be determined empirically. 
c     See Chapter 8 of Reference 2 for further information.
c
c  5. When IPARAM(1) = 0, and IDO = 3, the user needs to provide the 
c     NP = IPARAM(8) real and imaginary parts of the shifts in locations 
c         real part                  imaginary part
c         -----------------------    --------------
c     1   WORKL(IPNTR(14))           WORKL(IPNTR(14)+NP)
c     2   WORKL(IPNTR(14)+1)         WORKL(IPNTR(14)+NP+1)
c                        .                          .
c                        .                          .
c                        .                          .
c     NP  WORKL(IPNTR(14)+NP-1)      WORKL(IPNTR(14)+2*NP-1).
c
c     Only complex conjugate pairs of shifts may be applied and the pairs 
c     must be placed in consecutive locations. The real part of the 
c     eigenvalues of the current upper Hessenberg matrix are located in 
c     WORKL(IPNTR(6)) through WORKL(IPNTR(6)+NCV-1) and the imaginary part 
c     in WORKL(IPNTR(7)) through WORKL(IPNTR(7)+NCV-1). They are ordered
c     according to the order defined by WHICH. The complex conjugate
c     pairs are kept together and the associated Ritz estimates are located in
c     WORKL(IPNTR(8)), WORKL(IPNTR(8)+1), ... , WORKL(IPNTR(8)+NCV-1).
c
c-----------------------------------------------------------------------
c
c\Data Distribution Note: 
c
c  Fortran-D syntax:
c  ================
c  Double precision  resid(n), v(ldv,ncv), workd(3*n), workl(lworkl)
c  decompose  d1(n), d2(n,ncv)
c  align      resid(i) with d1(i)
c  align      v(i,j)   with d2(i,j)
c  align      workd(i) with d1(i)     range (1:n)
c  align      workd(i) with d1(i-n)   range (n+1:2*n)
c  align      workd(i) with d1(i-2*n) range (2*n+1:3*n)
c  distribute d1(block), d2(block,:)
c  replicated workl(lworkl)
c
c  Cray MPP syntax:
c  ===============
c  Double precision   resid(n), v(ldv,ncv), workd(n,3), workl(lworkl)
c  shared     resid(block), v(block,:), workd(block,:)
c  replicated workl(lworkl)
c  
c  CM2/CM5 syntax:
c  ==============
c  
c-----------------------------------------------------------------------
c
c     include   'ex-nonsym.doc'
c
c-----------------------------------------------------------------------
c
c\BeginLib
c
c\Local variables:
c     xxxxxx  real
c
c\References:
c  1. D.C. Sorensen, "Implicit Application of Polynomial Filters in
c     a k-Step Arnoldi Method", SIAM J. Matr. Anal. Apps., 13 (1992),
c     pp 357-385.
c  2. R.B. Lehoucq, "Analysis and Implementation of an Implicitly 
c     Restarted Arnoldi Iteration", Rice University Technical Report
c     TR95-13, Department of Computational and Applied Mathematics.
c  3. B.N. Parlett & Y. Saad, "Complex Shift and Invert Strategies for
c     Real Matrices", Linear Algebra and its Applications, vol 88/89,
c     pp 575-595, (1987).
c
c\Routines called:
c     dnaup2   ARPACK routine that implements the Implicitly Restarted
c             Arnoldi Iteration.
c     ivout   ARPACK utility routine that prints integers.
c     second  ARPACK utility routine for timing.
c     dvout    ARPACK utility routine that prints vectors.
c     dlamch   LAPACK routine that determines machine constants.
c
c\Author
c     Danny Sorensen               Phuong Vu
c     Richard Lehoucq              CRPC / Rice University
c     Dept. of Computational &     Houston, Texas
c     Applied Mathematics
c     Rice University           
c     Houston, Texas            
c 
c\Revision history:
c     12/16/93: Version '1.1'
c
c\SCCS Information: @(#) 
c FILE: naupd.F   SID: 2.8   DATE OF SID: 04/10/01   RELEASE: 2
 */

/* from dneupd.f distributed with arpack:
c\Name: dneupd 
c
c\Description: 
c
c  This subroutine returns the converged approximations to eigenvalues
c  of A*z = lambda*B*z and (optionally):
c
c      (1) The corresponding approximate eigenvectors;
c
c      (2) An orthonormal basis for the associated approximate
c          invariant subspace;
c
c      (3) Both.
c
c  There is negligible additional cost to obtain eigenvectors.  An orthonormal
c  basis is always computed.  There is an additional storage cost of n*nev
c  if both are requested (in this case a separate array Z must be supplied).
c
c  The approximate eigenvalues and eigenvectors of  A*z = lambda*B*z
c  are derived from approximate eigenvalues and eigenvectors of
c  of the linear operator OP prescribed by the MODE selection in the
c  call to DNAUPD .  DNAUPD  must be called before this routine is called.
c  These approximate eigenvalues and vectors are commonly called Ritz
c  values and Ritz vectors respectively.  They are referred to as such
c  in the comments that follow.  The computed orthonormal basis for the
c  invariant subspace corresponding to these Ritz values is referred to as a
c  Schur basis.
c
c  See documentation in the header of the subroutine DNAUPD  for 
c  definition of OP as well as other terms and the relation of computed
c  Ritz values and Ritz vectors of OP with respect to the given problem
c  A*z = lambda*B*z.  For a brief description, see definitions of 
c  IPARAM(7), MODE and WHICH in the documentation of DNAUPD .
c
c\Usage:
c  call dneupd  
c     ( RVEC, HOWMNY, SELECT, DR, DI, Z, LDZ, SIGMAR, SIGMAI, WORKEV, BMAT, 
c       N, WHICH, NEV, TOL, RESID, NCV, V, LDV, IPARAM, IPNTR, WORKD, WORKL, 
c       LWORKL, INFO )
c
c\Arguments:
c  RVEC    LOGICAL  (INPUT) 
c          Specifies whether a basis for the invariant subspace corresponding 
c          to the converged Ritz value approximations for the eigenproblem 
c          A*z = lambda*B*z is computed.
c
c             RVEC = .FALSE.     Compute Ritz values only.
c
c             RVEC = .TRUE.      Compute the Ritz vectors or Schur vectors.
c                                See Remarks below. 
c 
c  HOWMNY  Character*1  (INPUT) 
c          Specifies the form of the basis for the invariant subspace 
c          corresponding to the converged Ritz values that is to be computed.
c
c          = 'A': Compute NEV Ritz vectors; 
c          = 'P': Compute NEV Schur vectors;
c          = 'S': compute some of the Ritz vectors, specified
c                 by the logical array SELECT.
c
c  SELECT  Logical array of dimension NCV.  (INPUT)
c          If HOWMNY = 'S', SELECT specifies the Ritz vectors to be
c          computed. To select the Ritz vector corresponding to a
c          Ritz value (DR(j), DI(j)), SELECT(j) must be set to .TRUE.. 
c          If HOWMNY = 'A' or 'P', SELECT is used as internal workspace.
c
c  DR      Double precision  array of dimension NEV+1.  (OUTPUT)
c          If IPARAM(7) = 1,2 or 3 and SIGMAI=0.0  then on exit: DR contains 
c          the real part of the Ritz  approximations to the eigenvalues of 
c          A*z = lambda*B*z. 
c          If IPARAM(7) = 3, 4 and SIGMAI is not equal to zero, then on exit:
c          DR contains the real part of the Ritz values of OP computed by 
c          DNAUPD . A further computation must be performed by the user
c          to transform the Ritz values computed for OP by DNAUPD  to those
c          of the original system A*z = lambda*B*z. See remark 3 below.
c
c  DI      Double precision  array of dimension NEV+1.  (OUTPUT)
c          On exit, DI contains the imaginary part of the Ritz value 
c          approximations to the eigenvalues of A*z = lambda*B*z associated
c          with DR.
c
c          NOTE: When Ritz values are complex, they will come in complex 
c                conjugate pairs.  If eigenvectors are requested, the 
c                corresponding Ritz vectors will also come in conjugate 
c                pairs and the real and imaginary parts of these are 
c                represented in two consecutive columns of the array Z 
c                (see below).
c
c  Z       Double precision  N by NEV+1 array if RVEC = .TRUE. and HOWMNY = 'A'. (OUTPUT)
c          On exit, if RVEC = .TRUE. and HOWMNY = 'A', then the columns of 
c          Z represent approximate eigenvectors (Ritz vectors) corresponding 
c          to the NCONV=IPARAM(5) Ritz values for eigensystem 
c          A*z = lambda*B*z. 
c 
c          The complex Ritz vector associated with the Ritz value 
c          with positive imaginary part is stored in two consecutive 
c          columns.  The first column holds the real part of the Ritz 
c          vector and the second column holds the imaginary part.  The 
c          Ritz vector associated with the Ritz value with negative 
c          imaginary part is simply the complex conjugate of the Ritz vector 
c          associated with the positive imaginary part.
c
c          If  RVEC = .FALSE. or HOWMNY = 'P', then Z is not referenced.
c
c          NOTE: If if RVEC = .TRUE. and a Schur basis is not required,
c          the array Z may be set equal to first NEV+1 columns of the Arnoldi
c          basis array V computed by DNAUPD .  In this case the Arnoldi basis
c          will be destroyed and overwritten with the eigenvector basis.
c
c  LDZ     Integer.  (INPUT)
c          The leading dimension of the array Z.  If Ritz vectors are
c          desired, then  LDZ >= max( 1, N ).  In any case,  LDZ >= 1.
c
c  SIGMAR  Double precision   (INPUT)
c          If IPARAM(7) = 3 or 4, represents the real part of the shift. 
c          Not referenced if IPARAM(7) = 1 or 2.
c
c  SIGMAI  Double precision   (INPUT)
c          If IPARAM(7) = 3 or 4, represents the imaginary part of the shift. 
c          Not referenced if IPARAM(7) = 1 or 2. See remark 3 below.
c
c  WORKEV  Double precision  work array of dimension 3*NCV.  (WORKSPACE)
c
c  **** The remaining arguments MUST be the same as for the   ****
c  **** call to DNAUPD  that was just completed.               ****
c
c  NOTE: The remaining arguments
c
c           BMAT, N, WHICH, NEV, TOL, RESID, NCV, V, LDV, IPARAM, IPNTR,
c           WORKD, WORKL, LWORKL, INFO
c
c         must be passed directly to DNEUPD  following the last call
c         to DNAUPD .  These arguments MUST NOT BE MODIFIED between
c         the the last call to DNAUPD  and the call to DNEUPD .
c
c  Three of these parameters (V, WORKL, INFO) are also output parameters:
c
c  V       Double precision  N by NCV array.  (INPUT/OUTPUT)
c
c          Upon INPUT: the NCV columns of V contain the Arnoldi basis
c                      vectors for OP as constructed by DNAUPD  .
c
c          Upon OUTPUT: If RVEC = .TRUE. the first NCONV=IPARAM(5) columns
c                       contain approximate Schur vectors that span the
c                       desired invariant subspace.  See Remark 2 below.
c
c          NOTE: If the array Z has been set equal to first NEV+1 columns
c          of the array V and RVEC=.TRUE. and HOWMNY= 'A', then the
c          Arnoldi basis held by V has been overwritten by the desired
c          Ritz vectors.  If a separate array Z has been passed then
c          the first NCONV=IPARAM(5) columns of V will contain approximate
c          Schur vectors that span the desired invariant subspace.
c
c  WORKL   Double precision  work array of length LWORKL.  (OUTPUT/WORKSPACE)
c          WORKL(1:ncv*ncv+3*ncv) contains information obtained in
c          dnaupd .  They are not changed by dneupd .
c          WORKL(ncv*ncv+3*ncv+1:3*ncv*ncv+6*ncv) holds the
c          real and imaginary part of the untransformed Ritz values,
c          the upper quasi-triangular matrix for H, and the
c          associated matrix representation of the invariant subspace for H.
c
c          Note: IPNTR(9:13) contains the pointer into WORKL for addresses
c          of the above information computed by dneupd .
c          -------------------------------------------------------------
c          IPNTR(9):  pointer to the real part of the NCV RITZ values of the
c                     original system.
c          IPNTR(10): pointer to the imaginary part of the NCV RITZ values of
c                     the original system.
c          IPNTR(11): pointer to the NCV corresponding error bounds.
c          IPNTR(12): pointer to the NCV by NCV upper quasi-triangular
c                     Schur matrix for H.
c          IPNTR(13): pointer to the NCV by NCV matrix of eigenvectors
c                     of the upper Hessenberg matrix H. Only referenced by
c                     dneupd  if RVEC = .TRUE. See Remark 2 below.
c          -------------------------------------------------------------
c
c  INFO    Integer.  (OUTPUT)
c          Error flag on output.
c
c          =  0: Normal exit.
c
c          =  1: The Schur form computed by LAPACK routine dlahqr 
c                could not be reordered by LAPACK routine dtrsen .
c                Re-enter subroutine dneupd  with IPARAM(5)=NCV and 
c                increase the size of the arrays DR and DI to have 
c                dimension at least dimension NCV and allocate at least NCV 
c                columns for Z. NOTE: Not necessary if Z and V share 
c                the same space. Please notify the authors if this error
c                occurs.
c
c          = -1: N must be positive.
c          = -2: NEV must be positive.
c          = -3: NCV-NEV >= 2 and less than or equal to N.
c          = -5: WHICH must be one of 'LM', 'SM', 'LR', 'SR', 'LI', 'SI'
c          = -6: BMAT must be one of 'I' or 'G'.
c          = -7: Length of private work WORKL array is not sufficient.
c          = -8: Error return from calculation of a real Schur form.
c                Informational error from LAPACK routine dlahqr .
c          = -9: Error return from calculation of eigenvectors.
c                Informational error from LAPACK routine dtrevc .
c          = -10: IPARAM(7) must be 1,2,3,4.
c          = -11: IPARAM(7) = 1 and BMAT = 'G' are incompatible.
c          = -12: HOWMNY = 'S' not yet implemented
c          = -13: HOWMNY must be one of 'A' or 'P' if RVEC = .true.
c          = -14: DNAUPD  did not find any eigenvalues to sufficient
c                 accuracy.
c          = -15: DNEUPD got a different count of the number of converged
c                 Ritz values than DNAUPD got.  This indicates the user
c                 probably made an error in passing data from DNAUPD to
c                 DNEUPD or that the data was modified before entering
c                 DNEUPD
c
c\BeginLib
c
c\References:
c  1. D.C. Sorensen, "Implicit Application of Polynomial Filters in
c     a k-Step Arnoldi Method", SIAM J. Matr. Anal. Apps., 13 (1992),
c     pp 357-385.
c  2. R.B. Lehoucq, "Analysis and Implementation of an Implicitly 
c     Restarted Arnoldi Iteration", Rice University Technical Report
c     TR95-13, Department of Computational and Applied Mathematics.
c  3. B.N. Parlett & Y. Saad, "Complex Shift and Invert Strategies for
c     Real Matrices", Linear Algebra and its Applications, vol 88/89,
c     pp 575-595, (1987).
c
c\Routines called:
c     ivout   ARPACK utility routine that prints integers.
c     dmout    ARPACK utility routine that prints matrices
c     dvout    ARPACK utility routine that prints vectors.
c     dgeqr2   LAPACK routine that computes the QR factorization of 
c             a matrix.
c     dlacpy   LAPACK matrix copy routine.
c     dlahqr   LAPACK routine to compute the real Schur form of an
c             upper Hessenberg matrix.
c     dlamch   LAPACK routine that determines machine constants.
c     dlapy2   LAPACK routine to compute sqrt(x**2+y**2) carefully.
c     dlaset   LAPACK matrix initialization routine.
c     dorm2r   LAPACK routine that applies an orthogonal matrix in 
c             factored form.
c     dtrevc   LAPACK routine to compute the eigenvectors of a matrix
c             in upper quasi-triangular form.
c     dtrsen   LAPACK routine that re-orders the Schur form.
c     dtrmm    Level 3 BLAS matrix times an upper triangular matrix.
c     dger     Level 2 BLAS rank one update to a matrix.
c     dcopy    Level 1 BLAS that copies one vector to another .
c     ddot     Level 1 BLAS that computes the scalar product of two vectors.
c     dnrm2    Level 1 BLAS that computes the norm of a vector.
c     dscal    Level 1 BLAS that scales a vector.
c
c\Remarks
c
c  1. Currently only HOWMNY = 'A' and 'P' are implemented.
c
c     Let trans(X) denote the transpose of X.
c
c  2. Schur vectors are an orthogonal representation for the basis of
c     Ritz vectors. Thus, their numerical properties are often superior.
c     If RVEC = .TRUE. then the relationship
c             A * V(:,1:IPARAM(5)) = V(:,1:IPARAM(5)) * T, and
c     trans(V(:,1:IPARAM(5))) * V(:,1:IPARAM(5)) = I are approximately 
c     satisfied. Here T is the leading submatrix of order IPARAM(5) of the 
c     real upper quasi-triangular matrix stored workl(ipntr(12)). That is,
c     T is block upper triangular with 1-by-1 and 2-by-2 diagonal blocks; 
c     each 2-by-2 diagonal block has its diagonal elements equal and its
c     off-diagonal elements of opposite sign.  Corresponding to each 2-by-2
c     diagonal block is a complex conjugate pair of Ritz values. The real
c     Ritz values are stored on the diagonal of T.
c
c  3. If IPARAM(7) = 3 or 4 and SIGMAI is not equal zero, then the user must
c     form the IPARAM(5) Rayleigh quotients in order to transform the Ritz
c     values computed by DNAUPD  for OP to those of A*z = lambda*B*z. 
c     Set RVEC = .true. and HOWMNY = 'A', and
c     compute 
c           trans(Z(:,I)) * A * Z(:,I) if DI(I) = 0.
c     If DI(I) is not equal to zero and DI(I+1) = - D(I), 
c     then the desired real and imaginary parts of the Ritz value are
c           trans(Z(:,I)) * A * Z(:,I) +  trans(Z(:,I+1)) * A * Z(:,I+1),
c           trans(Z(:,I)) * A * Z(:,I+1) -  trans(Z(:,I+1)) * A * Z(:,I), 
c     respectively.
c     Another possibility is to set RVEC = .true. and HOWMNY = 'P' and
c     compute trans(V(:,1:IPARAM(5))) * A * V(:,1:IPARAM(5)) and then an upper
c     quasi-triangular matrix of order IPARAM(5) is computed. See remark
c     2 above.
c
c\Authors
c     Danny Sorensen               Phuong Vu
c     Richard Lehoucq              CRPC / Rice University 
c     Chao Yang                    Houston, Texas
c     Dept. of Computational &
c     Applied Mathematics          
c     Rice University           
c     Houston, Texas            
c 
c\SCCS Information: @(#) 
c FILE: neupd.F   SID: 2.7   DATE OF SID: 09/20/00   RELEASE: 2 
 */

int
main(void)
{
	// matrix
	NaiveMatrixHandler mh(5);
	mh(1, 1) = 2.;
	mh(1, 2) = 1.;
	mh(2, 1) = 1.;
	mh(2, 2) = 2.;
	mh(2, 3) = 1.;
	mh(3, 2) = 1.;
	mh(3, 3) = 2.;
	mh(3, 4) = 1.;
	mh(4, 3) = 1.;
	mh(4, 4) = 2.;
	mh(4, 5) = 1.;
	mh(5, 4) = 1.;
	mh(5, 5) = 2.;

	// shift
	doublereal SIGMA = 1.;
	doublereal SIGMAR = 0.;
	doublereal SIGMAI = 0.;

	// arpack-related vars
	integer IDO;		// 0 at first iteration; then set by dnaupd
	const char *BMAT;	// 'I' for standard problem
	integer N;		// size of problem
	const char *WHICH;	// "SM" to request smallest eigenvalues
	integer NEV;		// number of eigenvalues
	doublereal TOL;		// -1 to use machine precision
	std::vector<doublereal> RESID;	// residual vector (ignored if IDO==0)
	integer NCV;		// number of vectors in subspace
	std::vector<doublereal> V;	// Schur basis
	integer LDV;		// leading dimension of V (==N!)
	integer IPARAM[11] = { 0 };
	integer IPNTR[14] = { 0 };
	std::vector<doublereal> WORKD;
	std::vector<doublereal> WORKL;
	integer LWORKL;
	integer INFO;

	IDO = 0;
	BMAT = "I";
	N = mh.iGetNumRows();
	WHICH = "SM";
	NEV = 2;
	TOL = 0.;
	RESID.resize(N, 0.);
	NCV = 4;
	V.resize(N*NCV, 0.);
	LDV = N;
	IPARAM[0] = 1;
	IPARAM[2] = 300;
	IPARAM[3] = 1;
	IPARAM[6] = 1;			// mode...
	WORKD.resize(3*N, 0.);
	LWORKL = 3*NCV*NCV + 6*NCV;
	WORKL.resize(LWORKL, 0.);
	INFO = 0;

	int cnt = 0;
	do {
		std::cout << "before:" << std::endl
			<< "IDO=" << IDO << std::endl
			<< "BMAT=" << BMAT << std::endl
			<< "N=" << N << std::endl
			<< "WHICH=" << WHICH << std::endl
			<< "NEV=" << NEV << std::endl
			<< "TOL=" << TOL << std::endl
			<< "&RESID[0]=" << &RESID[0] << std::endl
			<< "NCV=" << NCV << std::endl
			<< "&V[0]=" << &V[0] << std::endl
			<< "LDV=" << LDV << std::endl
			<< "IPARAM=" << IPARAM[0]
				<< "," << IPARAM[1]
				<< "," << IPARAM[2]
				<< "," << IPARAM[3]
				<< "," << IPARAM[4]
				<< "," << IPARAM[5]
				<< "," << IPARAM[6]
				<< "," << IPARAM[7]
				<< "," << IPARAM[8]
				<< "," << IPARAM[9]
				<< "," << IPARAM[10]
				<< std::endl
			<< "IPNTR=" << IPNTR[0]
				<< "," << IPNTR[1]
				<< "," << IPNTR[2]
				<< "," << IPNTR[3]
				<< "," << IPNTR[4]
				<< "," << IPNTR[5]
				<< "," << IPNTR[6]
				<< "," << IPNTR[7]
				<< "," << IPNTR[8]
				<< "," << IPNTR[9]
				<< "," << IPNTR[10]
				<< "," << IPNTR[11]
				<< "," << IPNTR[12]
				<< "," << IPNTR[13]
				<< std::endl
			<< "&WORKD[0]=" << &WORKD[0] << std::endl
			<< "&WORKL[0]=" << &WORKL[0] << std::endl
			<< "LWORKL=" << LWORKL << std::endl
			<< "INFO=" << INFO << std::endl
			<< std::endl;

		__FC_DECL__(dnaupd)(&IDO, &BMAT[0], &N, &WHICH[0], &NEV,
			&TOL, &RESID[0], &NCV, &V[0], &LDV, &IPARAM[0], &IPNTR[0],
			&WORKD[0], &WORKL[0], &LWORKL, &INFO);

		std::cout << "after:" << std::endl
			<< "IDO=" << IDO << std::endl
			<< "BMAT=" << BMAT << std::endl
			<< "N=" << N << std::endl
			<< "WHICH=" << WHICH << std::endl
			<< "NEV=" << NEV << std::endl
			<< "TOL=" << TOL << std::endl
			<< "&RESID[0]=" << &RESID[0] << std::endl
			<< "NCV=" << NCV << std::endl
			<< "&V[0]=" << &V[0] << std::endl
			<< "LDV=" << LDV << std::endl
			<< "IPARAM=" << IPARAM[0]
				<< "," << IPARAM[1]
				<< "," << IPARAM[2]
				<< "," << IPARAM[3]
				<< "," << IPARAM[4]
				<< "," << IPARAM[5]
				<< "," << IPARAM[6]
				<< "," << IPARAM[7]
				<< "," << IPARAM[8]
				<< "," << IPARAM[9]
				<< "," << IPARAM[10]
				<< std::endl
			<< "IPNTR=" << IPNTR[0]
				<< "," << IPNTR[1]
				<< "," << IPNTR[2]
				<< "," << IPNTR[3]
				<< "," << IPNTR[4]
				<< "," << IPNTR[5]
				<< "," << IPNTR[6]
				<< "," << IPNTR[7]
				<< "," << IPNTR[8]
				<< "," << IPNTR[9]
				<< "," << IPNTR[10]
				<< "," << IPNTR[11]
				<< "," << IPNTR[12]
				<< "," << IPNTR[13]
				<< std::endl
			<< "&WORKD[0]=" << &WORKD[0] << std::endl
			<< "&WORKL[0]=" << &WORKL[0] << std::endl
			<< "LWORKL=" << LWORKL << std::endl
			<< "INFO=" << INFO << std::endl
			<< std::endl;

		std::cout << "cnt=" << cnt << ": IDO=" << IDO << ", INFO=" << INFO << std::endl;

		// compute Y = OP*X
		MyVectorHandler X(N, &WORKD[IPNTR[0] - 1]);
		MyVectorHandler Y(N, &WORKD[IPNTR[1] - 1]);

		Y = X;
		Y *= -SIGMA;
		mh.MatVecIncMul(Y, X);

		std::cout << "X:" << std::endl << X << std::endl;
		std::cout << "Y:" << std::endl << Y << std::endl;

		cnt++;
	} while (IDO == 1 || IDO == -1);

	if (INFO < 0) {
		std::cerr << "error" << std::endl;
		return 1;
	}

	logical RVEC = true;
	const char *HOWMNY = "A";
	std::vector<logical> SELECT(NCV);
	std::vector<doublereal> DR(NEV + 1);
	std::vector<doublereal> DI(NEV + 1);
	std::vector<doublereal> Z(N*(NEV + 1));
	integer LDZ = N;
	std::vector<doublereal> WORKEV(3*NCV);
	
	__FC_DECL__(dneupd)(&RVEC, &HOWMNY[0], &SELECT[0], &DR[0], &DI[0],
		&Z[0], &LDZ, &SIGMAR, &SIGMAI, &WORKEV[0],
		&BMAT[0], &N, &WHICH[0], &NEV,
		&TOL, &RESID[0], &NCV, &V[0], &LDV, &IPARAM[0], &IPNTR[0],
		&WORKD[0], &WORKL[0], &LWORKL, &INFO);

	bool bCmplx = false;
	bool bFirst;
	for (integer nv = 0; nv < NEV; nv++) {
		std::cout << "value " << std::setw(8) << nv << ":"
			<< std::setw(16) << DR[nv] + SIGMA
			<< (DI[nv] >= 0. ? " + " : " - ")
			<< std::setw(16) << std::abs(DI[nv]) << " i" << std::endl;

		std::cout << "vector " << std::setw(8) << nv << ":"
			<< std::endl;
		if (!bCmplx) {
			if (DI[nv] != 0.) {
				bCmplx = true;
				bFirst = true;
			}
		}

		if (!bCmplx) {
			for (integer idx = 0; idx < N; idx++) {
				std::cout
					<< std::setw(8) << idx << ":"
					<< std::setw(16) << Z[N*nv + idx]
					<< std::endl;
			}

		} else if (bFirst) {
			bFirst = false;

			for (integer idx = 0; idx < N; idx++) {
				doublereal d = Z[N*(nv + 1) + idx];
				std::cout
					<< std::setw(8) << idx << ":"
					<< std::setw(16) << Z[N*nv + idx]
					<< (d > 0. ? " + " : " - " )
					<< std::setw(16) << std::abs(d) << " i"
					<< std::endl;
			}

		} else {
			bCmplx = false;

			for (integer idx = 0; idx < N; idx++) {
				doublereal d = Z[N*nv + idx];
				std::cout
					<< std::setw(8) << idx << ":"
					<< std::setw(16) << Z[N*(nv - 1) + idx]
					<< (d > 0. ? " + " : " - " )
					<< std::setw(16) << std::abs(d) << " i"
					<< std::endl;
			}
		}
	}
	
	return 0;
}
