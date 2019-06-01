C MBDyn (C) is a multibody analysis code. 
C http://www.mbdyn.org
C 
C Copyright (C) 1996-2017
C 
C Pierangelo Masarati	<masarati@aero.polimi.it>
C Paolo Mantegazza	<mantegazza@aero.polimi.it>
C 
C Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
C via La Masa, 34 - 20156 Milano, Italy
C http://www.aero.polimi.it
C 
C Changing this copyright notice is forbidden.
C 
C This program is free software; you can redistribute it and/or modify
C it under the terms of the GNU General Public License as published by
C the Free Software Foundation (version 2 of the License).
C 
C 
C This program is distributed in the hope that it will be useful,
C but WITHOUT ANY WARRANTY; without even the implied warranty of
C MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
C GNU General Public License for more details.
C 
C You should have received a copy of the GNU General Public License
C along with this program; if not, write to the Free Software
C Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
C 
C Marks "rotor" IR as a non-rotating lifting surface that releases
C vortexes every DT seconds
C
C Subject: RE: Validation of MBDYN/CHARM interface
C Sender: dan@continuum-dynamics.com
C Date: 04/13/2011 07:41 PM
C
      SUBROUTINE FIXEDWING(IR, DT)
      IMPLICIT NONE
      INCLUDE "CVC.INC"
      INTEGER*4 IR
      REAL*8 DT
      KFWSIM(IR) = 1
      DTWSIM = DT
      RETURN
      END

