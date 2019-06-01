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

/*
 AUTHOR: Reinhard Resch <r.resch@secop.com>
        Copyright (C) 2015(-2017) all rights reserved.

        The copyright of this code is transferred
        to Pierangelo Masarati and Paolo Mantegazza
        for use in the software MBDyn as described
        in the GNU Public License version 2.1
*/

#ifdef HAVE_CONFIG_H
#include "mbconfig.h"
#endif

#include <drive.h>

#include <myassert.h>
#include <except.h>

#include <strnode.h>
#include <elem.h>
#include <mynewmem.h>
#include <dataman.h>

#include "module-nodedistdrive.h"

class NodeDistDriveCaller : public DriveCaller
{
public:
	NodeDistDriveCaller(const DriveHandler* pDH,
						const StructNode* pNode1,
						const Vec3& o1,
						const StructNode* pNode2,
						const Vec3& o2,
						const Vec3& e1);
	virtual ~NodeDistDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
	inline doublereal dGet(void) const;
	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	inline virtual doublereal dGetP(void) const;
	inline virtual doublereal dGetP(const doublereal& dVar) const;

private:
	const StructNode* const pNode1;
	const Vec3 o1;
	const StructNode* const pNode2;
	const Vec3 o2;
	const Vec3 e1;
};

struct NodeDistDCR : public DriveCallerRead {
	DriveCaller *
	Read(const DataManager* pDM, MBDynParser& HP, bool bDeferred);
};

DriveCaller *
NodeDistDCR::Read(const DataManager* pDM, MBDynParser& HP, bool bDeferred)
{
	NeedDM(pDM, HP, bDeferred, "node distance");

	const DriveHandler* pDrvHdl = 0;

	if (pDM != 0) {
		pDrvHdl = pDM->pGetDrvHdl();
	}

	DriveCaller *pDC = 0;

	/* driver legato ad un grado di liberta' nodale */
	if (pDM == 0) {
		silent_cerr("sorry, since the driver is not owned by a DataManager" << std::endl
			<< "no DOF dependent drivers are allowed;" << std::endl
			<< "aborting..." << std::endl);
		throw DataManager::ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	if ( !HP.IsKeyWord("node1") )
	{
		silent_cerr("node distance drive caller: keyword \"node1\" expected at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	StructNode* const pNode1 = pDM->ReadNode<StructNode, StructDispNode, Node::STRUCTURAL>(HP);

	const Vec3 o1 = HP.IsKeyWord("offset") ? HP.GetPosRel(ReferenceFrame(pNode1)) : Zero3;

	if ( !HP.IsKeyWord("node2") )
	{
		silent_cerr("node distance drive caller: keyword \"node2\" expected at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	StructNode* const pNode2 = pDM->ReadNode<StructNode, StructDispNode, Node::STRUCTURAL>(HP);

	const Vec3 o2 = HP.IsKeyWord("offset") ? HP.GetPosRel(ReferenceFrame(pNode2)) : Zero3;

	if ( !HP.IsKeyWord("direction") )
	{
		silent_cerr("node distance drive caller: keyword \"direction\" expected at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	Vec3 e1 = HP.GetVecRel(ReferenceFrame(pNode2));

	e1 /= e1.Norm();

	/* allocazione e creazione */
	SAFENEWWITHCONSTRUCTOR(pDC,
		NodeDistDriveCaller,
		NodeDistDriveCaller(pDrvHdl, pNode1, o1, pNode2, o2, e1));

	pDM->GetLogFile()
		<< "nodedistdrive: " << pDC->GetLabel()
		<< " (" << pDC->GetName() << ") "
		<< pNode1->GetLabel() << " "
		<< o1 << " "
		<< pNode2->GetLabel() << " "
		<< o2 << " "
		<< e1 << std::endl;

	return pDC;
}

doublereal
NodeDistDriveCaller::dGet(const doublereal& dVar) const
{
	return dGetP();
}

doublereal
NodeDistDriveCaller::dGet(void) const
{
	const Vec3& X1 = pNode1->GetXCurr();
	const Mat3x3& R1 = pNode1->GetRCurr();
	const Vec3& X2 = pNode2->GetXCurr();
	const Mat3x3& R2 = pNode2->GetRCurr();

	const doublereal dX = e1.Dot(R2.MulTV(X2 - X1 - R1 * o1) + o2);

	return dX;
}

bool NodeDistDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

doublereal NodeDistDriveCaller::dGetP(void) const
{
	const Vec3& X1 = pNode1->GetXCurr();
	const Vec3& X1Dot = pNode1->GetVCurr();
	const Mat3x3& R1 = pNode1->GetRCurr();
	const Vec3& omega1 = pNode1->GetWCurr();

	const Vec3& X2 = pNode2->GetXCurr();
	const Vec3& X2Dot = pNode2->GetVCurr();
	const Mat3x3& R2 = pNode2->GetRCurr();
	const Vec3& omega2 = pNode2->GetWCurr();

	const doublereal dXP = e1.Dot(R2.MulTV(-omega2.Cross(X2 - X1 - R1 * o1) + X2Dot - X1Dot - omega1.Cross(R1 * o1)));

	return dXP;
}

doublereal NodeDistDriveCaller::dGetP(const doublereal& dVar) const
{
	return dGetP();
}

NodeDistDriveCaller::NodeDistDriveCaller(
		const DriveHandler* pDH,
		const StructNode* pNode1Arg,
		const Vec3& o1Arg,
		const StructNode* pNode2Arg,
		const Vec3& o2Arg,
		const Vec3& e1Arg)
: DriveCaller(pDH),
  pNode1(pNode1Arg), o1(o1Arg),
  pNode2(pNode2Arg), o2(o2Arg),
  e1(e1Arg)
{
	NO_OP;
};

NodeDistDriveCaller::~NodeDistDriveCaller(void)
{
	NO_OP;
}

/* Copia */
DriveCaller*
NodeDistDriveCaller::pCopy(void) const
{
	DriveCaller* pDC = NULL;

	SAFENEWWITHCONSTRUCTOR(pDC,
			NodeDistDriveCaller,
			NodeDistDriveCaller(
				pDrvHdl,
				pNode1, o1,
				pNode2, o2,
				e1));

	return pDC;
}


/* Restart */
std::ostream&
NodeDistDriveCaller::Restart(std::ostream& out) const
{
	out << "node distance,"
		<< pNode1->GetLabel() << ","
		<< o1 << ","
		<< pNode2->GetLabel() << ","
		<< o2 << ","
		<< e1;

	return out;
}

bool nodedistdrive_set()
{
	DriveCallerRead	*rf = new NodeDistDCR;

	if (!SetDriveCallerData("node" "distance", rf)) {
		delete rf;
		return false;
	}

    return true;
}

#ifndef STATIC_MODULES

extern "C" int
module_init(const char *module_name, void *pdm, void *php)
{
	if (!nodedistdrive_set()) {
		silent_cerr("nodedistdrive: "
			"module_init(" << module_name << ") "
			"failed" << std::endl);
		return -1;
	}

	return 0;
}

#endif // ! STATIC_MODULES
