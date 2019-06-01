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

/* Classe di drivers che usano gradi di liberta' nodali
 * 
 * Il valore del drive viene fatto dipendere da un grado di liberta' nodale
 * in modo del tutto generale e trasparente. Ovvero in ingresso dati si
 * associa il drive al grado di liberta' j-esimo del nodo i-esimo, con 
 * verifiche di consistenza. In esecuzione, la funzione propria dGet() del 
 * drive restituisce il valore del grado di liberta' associato 
 * (o della derivata, se il grado di liberta' e' differenziale).
 */

#ifndef DOFDRIVE_H
#define DOFDRIVE_H

#include <drive.h>
#include <node.h>

class DofDriveCaller : public DriveCaller, public DriveOwner
{
protected:
	const ScalarDof SD;    
   
public:
	DofDriveCaller(const DriveHandler* pDH, const DriveCaller* pDC,
			const ScalarDof& sd);
	virtual ~DofDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;
   
	virtual std::ostream& Restart(std::ostream& out) const;
   
	inline doublereal dGet(const doublereal& dVar) const;
	inline doublereal dGet(void) const;
};

inline doublereal
DofDriveCaller::dGet(const doublereal& dVar) const
{
	silent_cerr("warning, possible improper call of dof drive "
		"with real argument" << std::endl);
	return DriveOwner::pGetDriveCaller()->dGet(dVar);
}

inline doublereal
DofDriveCaller::dGet(void) const
{
	return pGetDriveCaller()->dGet(SD.pNode->dGetDofValue(1, SD.iOrder));
}

#endif /* DOFDRIVE_H */

