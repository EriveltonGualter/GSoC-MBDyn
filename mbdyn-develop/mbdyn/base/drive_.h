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

/* drivers */

#ifndef DRIVE__H
#define DRIVE__H

/* include generali */
#include "parser.h"
#include <sstream>
#ifndef DO_NOT_USE_EE
#include <thread>
#endif // DO_NOT_USE_EE

/* include per il debug */
#include "myassert.h"
#include "mynewmem.h"
#include "except.h"

/* include del programma */
#include "mathp.h"
#include "output.h"
#include "withlab.h"

#include "drive.h"
#include "filedrv.h"


/* StringDriveCaller - begin */

class StringDriveCaller : public DriveCaller {
private:
	// we keep the string just in case...
	const std::string sEvalStr;

#ifndef DO_NOT_USE_EE
	// "smart" shared pointer; needs -std=c++11

	// TODO: actually, all drive callers could be wrapped in shared pointers
	class SharedExpr : public std::enable_shared_from_this<SharedExpr> {
	private:
		const ExpressionElement *m_expr;
	public:
		SharedExpr(const ExpressionElement *expr) : m_expr(expr) {};
		~SharedExpr(void) { delete m_expr; };
		const ExpressionElement *Get(void) const { return m_expr; };
        	std::shared_ptr<const SharedExpr> pCopy(void) const { return shared_from_this(); };
	};

	typedef std::shared_ptr<const SharedExpr> SharedExprPtr_t;

	SharedExprPtr_t m_expr;
#endif // DO_NOT_USE_EE

public:
#ifndef DO_NOT_USE_EE
	StringDriveCaller(const DriveHandler* pDH, const std::string& sTmpStr, const ExpressionElement *expr);
	StringDriveCaller(const DriveHandler* pDH, const std::string& sTmpStr, SharedExprPtr_t expr);
#else // ! USE_EE
	StringDriveCaller(const DriveHandler* pDH, const std::string& sTmpStr);
#endif // ! USE_EE

	~StringDriveCaller(void);

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	inline doublereal dGet(const doublereal& dVar) const;
	inline doublereal dGet(void) const;
};

inline doublereal
StringDriveCaller::dGet(const doublereal& dVar) const
{
	DriveCaller::pDrvHdl->SetVar(dVar);

	return dGet();
}

inline doublereal
StringDriveCaller::dGet(void) const
{
#ifndef DO_NOT_USE_EE
	doublereal val;
	try {
		val = m_expr->Get()->Eval().GetReal();
	} catch (MBDynErrBase& e) {
		silent_cerr("StringDriveCaller::dGet(): " << e.what() << std::endl);
		throw e;
	}

	return val;
#else // ! USE_EE
	std::istringstream in(sEvalStr);
	InputStream In(in);

	return DriveCaller::pDrvHdl->dGet(In);
#endif // ! USE_EE
}

/* StringDriveCaller - end */


/* TimeDriveCaller - begin */

class TimeDriveCaller : public DriveCaller {
public:
	TimeDriveCaller(const DriveHandler* pDH);
	virtual ~TimeDriveCaller(void);

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	inline doublereal dGet(const doublereal& dVar) const;
	inline doublereal dGet(void) const;

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
	virtual inline doublereal dGetP(void) const;
};

inline doublereal
TimeDriveCaller::dGet(const doublereal& dVar) const
{
	return dVar;
}

inline doublereal
TimeDriveCaller::dGet(void) const
{
	return pDrvHdl->dGetTime();
}

inline bool
TimeDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
TimeDriveCaller::dGetP(const doublereal& /* dVar */ ) const
{
	return 1.;
}

inline doublereal 
TimeDriveCaller::dGetP(void) const
{
	return 1.;
}

/* TimeDriveCaller - end */

/* TimeStepDriveCaller - begin */

class TimeStepDriveCaller : public DriveCaller {
public:
	TimeStepDriveCaller(const DriveHandler* pDH);
	virtual ~TimeStepDriveCaller(void);

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	inline doublereal dGet(const doublereal& dVar) const;
	inline doublereal dGet(void) const;
};

inline doublereal
TimeStepDriveCaller::dGet(const doublereal& dVar) const
{
	return dVar;
}

inline doublereal
TimeStepDriveCaller::dGet(void) const
{
	return pDrvHdl->dGetTimeStep();
}

/* TimeStepDriveCaller - end */


/* MultDriveCaller - begin */

class MultDriveCaller : public DriveCaller {
protected:
	DriveOwner DO1, DO2;

public:
	MultDriveCaller(const DriveHandler* pDH,
		const DriveCaller *pDC1, const DriveCaller *pDC2);
	virtual ~MultDriveCaller(void);

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	inline doublereal dGet(void) const;
	inline doublereal dGet(const doublereal& dVar) const;

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
};

inline doublereal
MultDriveCaller::dGet(void) const
{
	return DO1.pGetDriveCaller()->dGet()*DO2.pGetDriveCaller()->dGet();
}

inline doublereal
MultDriveCaller::dGet(const doublereal& dVar) const
{
	return DO1.pGetDriveCaller()->dGet(dVar)*DO2.pGetDriveCaller()->dGet(dVar);
}

inline bool
MultDriveCaller::bIsDifferentiable(void) const
{
	return DO1.pGetDriveCaller()->bIsDifferentiable()
		&& DO2.pGetDriveCaller()->bIsDifferentiable();
}

inline doublereal 
MultDriveCaller::dGetP(const doublereal& dVar) const
{
	return DO1.pGetDriveCaller()->dGetP(dVar)*DO2.pGetDriveCaller()->dGet(dVar)
		+ DO1.pGetDriveCaller()->dGet(dVar)*DO2.pGetDriveCaller()->dGetP(dVar);
}

/* MultDriveCaller - end */

/* LinearDriveCaller - begin */

class LinearDriveCaller : public DriveCaller {
private:
	doublereal dC0;
	doublereal dC1;

public:
	LinearDriveCaller(const DriveHandler* pDH, doublereal d0, doublereal d1);
	virtual ~LinearDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
LinearDriveCaller::dGet(const doublereal& dVar) const
{
	return dC0 + dVar*dC1;
}

inline bool
LinearDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
LinearDriveCaller::dGetP(const doublereal& /* dVar */ ) const
{
	return dC1;
}

/* LinearDriveCaller - end */


/* ParabolicDriveCaller - begin */

class ParabolicDriveCaller : public DriveCaller {
private:
	doublereal dC0;
	doublereal dC1;
	doublereal dC2;

public:
	ParabolicDriveCaller(const DriveHandler* pDH,
		doublereal d0, doublereal d1, doublereal d2);
	virtual ~ParabolicDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
ParabolicDriveCaller::dGet(const doublereal& dVar) const
{
	return dC0 + dVar*(dC1 + dVar*dC2);
}

inline bool
ParabolicDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
ParabolicDriveCaller::dGetP(const doublereal& dVar) const
{
	return dC1 + dVar*2*dC2;
}

/* ParabolicDriveCaller - end */


/* CubicDriveCaller - begin */

class CubicDriveCaller : public DriveCaller {
private:
	doublereal dC0;
	doublereal dC1;
	doublereal dC2;
	doublereal dC3;

public:
	CubicDriveCaller(const DriveHandler* pDH,
		doublereal d0, doublereal d1,
		doublereal d2, doublereal d3);
	virtual ~CubicDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
CubicDriveCaller::dGet(const doublereal& dVar) const
{
	return dC0 + dVar*(dC1 + dVar*(dC2 + dVar*dC3));
}

inline bool
CubicDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
CubicDriveCaller::dGetP(const doublereal& dVar) const
{
	return dC1 + dVar*(2*dC2 + dVar*3*dC3);
}

/* CubicDriveCaller - end */


/* StepDriveCaller - begin */

class StepDriveCaller : public DriveCaller {
private:
	doublereal dStepTime;
	doublereal dStepValuePlusInitialValue;
	doublereal dInitialValue;

public:
	StepDriveCaller(const DriveHandler* pDH,
		doublereal d1, doublereal d2, doublereal d3);
	~StepDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
StepDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar > dStepTime) {
		return dStepValuePlusInitialValue;
	}

	if (dVar < dStepTime) {
		return dInitialValue;
	}

	/* else if dVar == dStepTime */
	return dStepValuePlusInitialValue/2.;
}

inline bool
StepDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
StepDriveCaller::dGetP(const doublereal& dVar) const
{
	/* FIXME: what if we get exactly to the step time? */
	if (dVar == dStepTime) {
		silent_cerr("singularity in step drive derivative at " << dVar << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	return 0.;
}

/* StepDriveCaller - end */


/* DoubleStepDriveCaller - begin */

class DoubleStepDriveCaller : public DriveCaller {
private:
	doublereal dStepTime;
	doublereal dStepValuePlusInitialValue;
	doublereal dEndStepTime;
	doublereal dInitialValue;

public:
	DoubleStepDriveCaller(const DriveHandler* pDH,
		doublereal d1, doublereal d2,
		doublereal d3, doublereal d4);
	~DoubleStepDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
DoubleStepDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar > dStepTime && dVar < dEndStepTime) {
		return dStepValuePlusInitialValue;
	}

	if (dVar < dStepTime || dVar > dEndStepTime) {
		return dInitialValue;
	}

	/* else if dVar == dStepTime || dVar == dEndStepTime */
	return dStepValuePlusInitialValue/2.;
}

inline bool
DoubleStepDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
DoubleStepDriveCaller::dGetP(const doublereal& dVar) const
{
	/* FIXME: what if we get exactly to the step time? */
	if (dVar == dStepTime || dVar == dEndStepTime) {
		silent_cerr("singularity in double step drive derivative at " << dVar << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	return 0.;
}

/* DoubleStepDriveCaller - end */


/* RampDriveCaller - begin */

class RampDriveCaller : public DriveCaller {
private:
	doublereal dSlope;
	doublereal dStartTime;
	doublereal dEndTime;
	doublereal dInitialValue;

public:
	RampDriveCaller(const DriveHandler* pDH,
		doublereal d1, doublereal d2, doublereal d3, doublereal d4);
	~RampDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
RampDriveCaller::dGet(const doublereal& dVar) const
{
	doublereal dVal;

	dVal = dInitialValue;
	if (dVar > dStartTime) {
		doublereal dEnd;

		if (dVar > dEndTime) {
			dEnd = dEndTime;

		} else {
			dEnd = dVar;
		}

		dVal += dSlope*(dEnd - dStartTime);
	}

	return dVal;
}

inline bool
RampDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
RampDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dStartTime || dVar > dEndTime) {
		return 0.;
	}

	if (dVar == dStartTime || dVar == dEndTime) {
		return dSlope/2.;
	}

	return dSlope;
}

/* RampDriveCaller - end */


/* DoubleRampDriveCaller - begin */

/* note: can be obtained with an array of two ramps */

class DoubleRampDriveCaller : public DriveCaller {
private:
	doublereal dAscendingSlope;
	doublereal dAscendingStartTime;
	doublereal dAscendingEndTime;
	doublereal dDescendingSlope;
	doublereal dDescendingStartTime;
	doublereal dDescendingEndTime;
	doublereal dInitialValue;

public:
	DoubleRampDriveCaller(const DriveHandler* pDH,
		doublereal d1, doublereal d2, doublereal d3,
		doublereal d4, doublereal d5, doublereal d6,
		doublereal d7);
	~DoubleRampDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
DoubleRampDriveCaller::dGet(const doublereal& dVar) const
{
	doublereal dVal;

	dVal = dInitialValue;
	if (dVar > dAscendingStartTime) {
		doublereal dEnd;

		if (dVar > dAscendingEndTime) {
			dEnd = dAscendingEndTime;

		} else {
			dEnd = dVar;
		}

		dVal += dAscendingSlope*(dEnd - dAscendingStartTime);

		if (dVar > dDescendingStartTime) {
			if (dVar > dDescendingEndTime) {
				dEnd = dDescendingEndTime;

			} else {
				dEnd = dVar;
			}

			dVal += dDescendingSlope*(dEnd - dDescendingStartTime);
		}
	}

	return dVal;
}

inline bool
DoubleRampDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
DoubleRampDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dAscendingStartTime || dVar > dDescendingEndTime
		|| (dVar > dAscendingEndTime && dVar < dDescendingStartTime))
	{
		return 0.;
	}

	if (dVar == dAscendingStartTime || dVar == dAscendingEndTime) {
		return dAscendingSlope/2.;
	}

	if (dVar == dDescendingStartTime || dVar == dDescendingEndTime) {
		return dDescendingSlope/2.;
	}

	if (dVar < dAscendingEndTime) {
		return dAscendingSlope;
	}

	return dDescendingSlope;
}

/* DoubleRampDriveCaller - end */


/* SineDriveCaller - begin */

class SineDriveCaller : public DriveCaller {
private:
	doublereal dStartTime;
	doublereal dOmega;
	doublereal dAmplitude;
	integer iNumCycles;
	doublereal dInitialValue;
	doublereal dEndTime;
	doublereal dFinalValue;
	bool bNeverEnd;

public:
	SineDriveCaller(const DriveHandler* pDH,
		doublereal d1, doublereal d2, doublereal d3,
		integer iNumCyc, doublereal d4);
	~SineDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
SineDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar <= dStartTime) {
		return dInitialValue;
	}

	if (bNeverEnd || dVar < dEndTime) {
		return dInitialValue + dAmplitude*sin(dOmega*(dVar - dStartTime));
	}

	/* else if dVar > dEndTime */
	return dFinalValue;
}

inline bool
SineDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
SineDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dStartTime) {
		return 0.;
	}

	if (!bNeverEnd && dVar > dEndTime) {
		return 0.;
	}

	doublereal dVal = dAmplitude*dOmega*cos(dOmega*(dVar - dStartTime));

	if (dVar == dStartTime || (!bNeverEnd && dVar == dEndTime)) {
		dVal /= 2.;
	}

	return dVal;
}

/* SineDriveCaller - end */


/* CosineDriveCaller - begin */

class CosineDriveCaller : public DriveCaller {
private:
	doublereal dStartTime;
	doublereal dOmega;
	doublereal dAmplitude;
	integer iNumCycles;
	doublereal dInitialValue;
	doublereal dEndTime;
	doublereal dFinalValue;
	bool bNeverEnd;

public:
	CosineDriveCaller(const DriveHandler* pDH,
		doublereal d1, doublereal d2, doublereal d3,
		integer iNumCyc, doublereal d4);
	~CosineDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
CosineDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar < dStartTime) {
		return dInitialValue;
	}

	if (bNeverEnd || dVar < dEndTime) {
		return dInitialValue + dAmplitude*(1. - cos(dOmega*(dVar - dStartTime)));
	}

	/* else if dTime > dEndTime */
	return dFinalValue;
}

inline bool
CosineDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
CosineDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dStartTime) {
		return 0.;
	}

	if (!bNeverEnd && dVar > dEndTime) {
		return 0.;
	}

	return dAmplitude*dOmega*sin(dOmega*(dVar - dStartTime));
}

/* CosineDriveCaller - end */


/* TanhDriveCaller - begin */

class TanhDriveCaller : public DriveCaller {
private:
	doublereal dStart;
	doublereal dA;
	doublereal dB;
	doublereal dInitialValue;

public:
	TanhDriveCaller(const DriveHandler* pDH,
		doublereal ds, doublereal da, doublereal db, doublereal di);
	~TanhDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
};

inline doublereal
TanhDriveCaller::dGet(const doublereal& dVar) const
{
	return dInitialValue + dA*std::tanh(dB*(dVar - dStart));
}

inline bool
TanhDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
TanhDriveCaller::dGetP(const doublereal& dVar) const
{
	return dA*dB*(1. - std::tanh(dB*(dVar - dStart)));
}

/* TanhDriveCaller - end */


/* FourierSeriesDriveCaller - begin */

class FourierSeriesDriveCaller : public DriveCaller {
private:
	doublereal dStartTime;
	doublereal dOmega;
	mutable std::vector<doublereal> amplitudes;
	integer iNumCycles;
	doublereal dInitialValue;
	doublereal dEndTime;
	bool bNeverEnd;

public:
	FourierSeriesDriveCaller(const DriveHandler* pDH,
		doublereal dStartTime,
		doublereal dOmega,
		std::vector<doublereal>& a,
		integer iNumCyc,
		doublereal dInitialValue);
	~FourierSeriesDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
FourierSeriesDriveCaller::dGet(const doublereal& dVar) const
{
	doublereal d = dInitialValue;

	if (dVar >= dStartTime && (bNeverEnd || dVar < dEndTime)) {
		doublereal t = dVar - dStartTime;

		d += amplitudes[0];

		for (unsigned i = 2; i < amplitudes.size(); i += 2 ) {
			doublereal theta = (i/2)*dOmega*t;

			d += amplitudes[i - 1]*cos(theta) + amplitudes[i]*sin(theta);
		}
	}

	return d;
}

inline bool
FourierSeriesDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
FourierSeriesDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dStartTime) {
		return 0.;
	}

	if (!bNeverEnd && dVar > dEndTime) {
		return 0.;
	}

	doublereal t = dVar - dStartTime;
	doublereal dVal = 0.;
	bool bSingular = (amplitudes[0] != 0);
	for (unsigned i = 2; i < amplitudes.size(); i += 2 ) {
		doublereal omega = (i/2)*dOmega;
		doublereal theta = omega*t;

		dVal += omega*(-amplitudes[i - 1]*sin(theta) + amplitudes[i]*cos(theta));

		if (amplitudes[i - 1] != 0) {
			bSingular = true;
		}
	}

	if (dVar == dStartTime || (!bNeverEnd && dVar == dEndTime)) {
		if (bSingular) {
			silent_cerr("singularity in fourier series drive derivative at " << dVar << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		dVal /= 2.;
	}

	return dVal;
}

/* FourierSeriesDriveCaller - end */


/* FreqSweepDriveCaller - begin */

class FreqSweepDriveCaller : public DriveCaller {
private:
	doublereal dStartTime;
	const DriveCaller *pOmega;
	const DriveCaller *pAmplitude;
	doublereal dInitialValue;
	doublereal dEndTime;
	doublereal dFinalValue;
	bool bNeverEnd;

public:
	FreqSweepDriveCaller(const DriveHandler* pDH,
		doublereal d1,
		const DriveCaller* pOmega,
		const DriveCaller* pAmplitude,
		doublereal d2,
		doublereal d3,
		doublereal d4);
	~FreqSweepDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
FreqSweepDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar <= dStartTime) {
		return dInitialValue;
	}

	if (bNeverEnd || dVar < dEndTime) {
		return dInitialValue + pAmplitude->dGet(dVar)*sin(pOmega->dGet(dVar)*(dVar - dStartTime));
	}

	/* else if dVar > dEndTime */
	return dFinalValue;
}

inline bool
FreqSweepDriveCaller::bIsDifferentiable(void) const
{
	return pAmplitude->bIsDifferentiable() && pOmega->bIsDifferentiable();
}

inline doublereal 
FreqSweepDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dStartTime || (!bNeverEnd && dVar > dEndTime)) {
		return 0.;
	}

	doublereal A = pAmplitude->dGet(dVar);
	doublereal w = pOmega->dGet(dVar);
	doublereal AP = pAmplitude->dGetP(dVar);
	doublereal wP = pOmega->dGetP(dVar);
	doublereal t = dVar - dStartTime;

	doublereal dVal = AP*sin(w*t) + A*cos(w*t)*(wP*t + w);
	if (dVar == dStartTime || (!bNeverEnd && dVar == dEndTime)) {
		dVal /= 2.;
	}

	return dVal;
}

/* FreqSweepDriveCaller - end */


/* ExpDriveCaller - begin */

class ExpDriveCaller : public DriveCaller {
private:
	doublereal dAmplitude;
	doublereal dTimeConst;
	doublereal dStartTime;
	doublereal dInitialValue;

public:
	ExpDriveCaller(const DriveHandler* pDH,
		doublereal dA, doublereal dT,
		doublereal dS, doublereal dI);
	virtual ~ExpDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
ExpDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar <= dStartTime) {
		return dInitialValue;
	}

	return dInitialValue
		+ dAmplitude*(1. - exp((dStartTime - dVar)/dTimeConst));
}

inline bool
ExpDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
ExpDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dStartTime) {
		return 0.;
	}

	doublereal dVal = -dAmplitude/dTimeConst*exp((dStartTime - dVar)/dTimeConst);
	if (dVal == dStartTime) {
		dVal /= 2.;
	}

	return dVal;
}

/* ExpDriveCaller - end */


/* RandDriveCaller - begin */

class RandDriveCaller : public DriveCaller {
private:
	long int iBase;
	doublereal dAmplitude;
	doublereal dRefVal;
	doublereal dStartTime;
	doublereal dEndTime;
	integer iSteps;
	integer iRandDriveNumber;

public:
	RandDriveCaller(const DriveHandler* pDH,
		doublereal dA, doublereal dR,
		doublereal dS, doublereal dE, integer iS);
	virtual ~RandDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif
};

inline doublereal
RandDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar < dStartTime || dVar > dEndTime) {
		return dRefVal;
	}

	doublereal dRand = doublereal((((unsigned long)pDrvHdl->iGetRand(iRandDriveNumber)) + iBase)%RAND_MAX);
	return dRefVal + dAmplitude*(2.*dRand/doublereal(RAND_MAX) - 1.);
}

/* RandDriveCaller - end */


/* MeterDriveCaller - begin */

class MeterDriveCaller : public DriveCaller {
private:
	doublereal dStartTime;
	doublereal dEndTime;
	integer iSteps;
	integer iMeterDriveNumber;

public:
	MeterDriveCaller(const DriveHandler* pDH,
		doublereal dS, doublereal dE, integer iS);
	virtual ~MeterDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	inline integer iGetSteps(void) const;
};

inline doublereal
MeterDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar < dStartTime || dVar > dEndTime) {
		return 0.;
	}

	return doublereal(pDrvHdl->bGetMeter(iMeterDriveNumber));
}

inline integer
MeterDriveCaller::iGetSteps(void) const
{
	return iSteps;
}

/* MeterDriveCaller - end */


/* ClosestNextDriveCaller - begin */

class ClosestNextDriveCaller : public DriveCaller {
private:
	doublereal dStartTime;
	doublereal dEndTime;
	const DriveCaller *pIncrement;
	integer iDriveNumber;

public:
	ClosestNextDriveCaller(const DriveHandler* pDH,
		doublereal dS, doublereal dE,
		const DriveCaller *pIncrement);
	virtual ~ClosestNextDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
};

inline doublereal
ClosestNextDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar < dStartTime || dVar > dEndTime) {
		return 0.;
	}

	return doublereal(pDrvHdl->bGetClosestNext(iDriveNumber));
}

/* ClosestNextDriveCaller - end */


/* DiscreteFilterDriveCaller - begin */

class DiscreteFilterDriveCaller : public DriveCaller {
private:
	integer iDiscreteFilterDriveNumber;

public:
	DiscreteFilterDriveCaller(const DriveHandler* pDH,
		DriveCaller *pDC,
		const std::vector<doublereal>& a,
		doublereal b0,
		const std::vector<doublereal>& b);
	virtual ~DiscreteFilterDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(void) const;
	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif
};

inline doublereal
DiscreteFilterDriveCaller::dGet(void) const
{
	return pDrvHdl->dGetDiscreteFilter(iDiscreteFilterDriveNumber);
}

inline doublereal
DiscreteFilterDriveCaller::dGet(const doublereal& dVar) const
{
	return pDrvHdl->dGetDiscreteFilter(iDiscreteFilterDriveNumber, dVar);
}

/* RandDriveCaller - end */


/* DirectDriveCaller - begin */

class DirectDriveCaller : public DriveCaller {
public:
	DirectDriveCaller(const DriveHandler* pDH);
	virtual ~DirectDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
#if 0
	inline doublereal dGet(void) const;
#endif

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
};

inline doublereal
DirectDriveCaller::dGet(const doublereal& dVar) const
{
	return dVar;
}

inline bool
DirectDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
DirectDriveCaller::dGetP(const doublereal& dVar) const
{
	return 1.;
}

/* DirectDriveCaller - end */


/* PiecewiseLinearDriveCaller - begin */

class PiecewiseLinearDriveCaller : public DriveCaller {
private:
	unsigned int iNumPoints;
	doublereal *pPoints;
	doublereal *pVals;

public:
	PiecewiseLinearDriveCaller(const DriveHandler* pDH,
			unsigned int i, doublereal *p);
	virtual ~PiecewiseLinearDriveCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
PiecewiseLinearDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar <= pPoints[0]) {
		return pVals[0];
	}

	if (dVar >= pPoints[iNumPoints - 1]) {
		return pVals[iNumPoints - 1];
	}

	for (unsigned int i = 1; i < iNumPoints; i++) {
		if (dVar == pPoints[i]) {
			return pVals[i];
		}
		if (dVar < pPoints[i]) {
			doublereal dx = pPoints[i] - pPoints[i - 1];
			return ((dVar - pPoints[i - 1])*pVals[i]
				+ (pPoints[i] - dVar)*pVals[i - 1])/dx;
		}
	}

	throw ErrGeneric(MBDYN_EXCEPT_ARGS);
}

inline bool
PiecewiseLinearDriveCaller::bIsDifferentiable(void) const
{
	return true;
}

inline doublereal 
PiecewiseLinearDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < pPoints[0] || dVar > pPoints[iNumPoints - 1]) {
		return 0.;
	}

	if (dVar == pPoints[0]) {
		return (pVals[1] - pVals[0])/(pPoints[1] - pPoints[0])/2.;
	}

	if (dVar == pPoints[iNumPoints - 1]) {
		return (pVals[iNumPoints - 1] - pVals[iNumPoints - 2])/(pPoints[iNumPoints - 1] - pPoints[iNumPoints - 2])/2.;
	}

	for (unsigned int i = 1; i < iNumPoints; i++) {
		if (dVar == pPoints[i]) {
			doublereal dS1 = (pVals[i] - pVals[i - 1])/(pPoints[i] - pPoints[i - 1]);
			doublereal dS2 = (pVals[i + 1] - pVals[i])/(pPoints[i + 1] - pPoints[i]);

			return (dS1 + dS2)/2.;
		}

		if (dVar < pPoints[i]) {
			return (pVals[i] - pVals[i - 1])/(pPoints[i] - pPoints[i - 1]);
		}
	}

	throw ErrGeneric(MBDYN_EXCEPT_ARGS);
}

/* PiecewiseLinearDriveCaller - end */

/* DriveArrayCaller - begin */

class DriveArrayCaller : public DriveCaller {
public:
	typedef std::vector<const DriveCaller *> dcv_t;

private:
	dcv_t  m_dc;

public:
	DriveArrayCaller(const DriveHandler* pDH, dcv_t& DC);
	virtual ~DriveArrayCaller(void);

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	inline doublereal dGet(const doublereal& dVar) const;
	inline doublereal dGet(void) const;

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
#if 0
	virtual inline doublereal dGetP(void) const;
#endif
};

inline doublereal
DriveArrayCaller::dGet(const doublereal& dVar) const
{
	doublereal d = 0.;

	for (dcv_t::const_iterator i = m_dc.begin(); i != m_dc.end(); ++i) {
		ASSERT(*i != 0);

		d += (*i)->dGet(dVar);
	}

	return d;
}

inline doublereal
DriveArrayCaller::dGet(void) const
{
	doublereal d = 0.;

	for (dcv_t::const_iterator i = m_dc.begin(); i != m_dc.end(); ++i) {
		ASSERT(*i != 0);

		d += (*i)->dGet();
	}

	return d;
}

inline bool
DriveArrayCaller::bIsDifferentiable(void) const
{
	for (dcv_t::const_iterator i = m_dc.begin(); i != m_dc.end(); ++i) {
		ASSERT(*i != 0);

		if (!(*i)->bIsDifferentiable()) {
			return false;
		}
	}

	return true;
}

inline doublereal 
DriveArrayCaller::dGetP(const doublereal& dVar) const
{
	doublereal dP = 0.;

	for (dcv_t::const_iterator i = m_dc.begin(); i != m_dc.end(); ++i) {
		ASSERT(*i != 0);
		ASSERT((*i)->bIsDifferentiable());

		dP += (*i)->dGetP(dVar);
	}

	return dP;
}

/* DriveArrayCaller - end */

/* PeriodicDriveCaller - begin */

class PeriodicDriveCaller : public DriveCaller {
protected:
	DriveOwner DO;
	doublereal dT0;
	doublereal dPeriod;

public:
	PeriodicDriveCaller(const DriveHandler* pDH,
		const DriveCaller *pDC, doublereal dT0, doublereal dPeriod);
	virtual ~PeriodicDriveCaller(void);

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	inline doublereal dGet(void) const;
	inline doublereal dGet(const doublereal& dVar) const;

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
};

inline doublereal
PeriodicDriveCaller::dGet(void) const
{
	doublereal dTime = pDrvHdl->dGetTime();
	if (dTime < dT0) {
		return 0.;
	}
	dTime -= dT0;
	dTime -= floor(dTime/dPeriod)*dPeriod;
	return DO.pGetDriveCaller()->dGet(dTime);
}

inline doublereal
PeriodicDriveCaller::dGet(const doublereal& dVar) const
{
	if (dVar < dT0) {
		return 0.;
	}
	doublereal dTime = dVar - dT0;
	dTime -= floor(dTime/dPeriod)*dPeriod;
	return DO.pGetDriveCaller()->dGet(dTime);
}

inline bool
PeriodicDriveCaller::bIsDifferentiable(void) const
{
	return DO.pGetDriveCaller()->bIsDifferentiable();
}

inline doublereal 
PeriodicDriveCaller::dGetP(const doublereal& dVar) const
{
	if (dVar < dT0) {
		return 0.;
	}
	doublereal dTime = dVar - dT0;
	dTime -= floor(dTime/dPeriod)*dPeriod;
	return DO.pGetDriveCaller()->dGetP(dTime);
}

/* PeriodicDriveCaller - end */

/* PostponedDriveCaller - begin */

class PostponedDriveCaller : public DriveCaller {
protected:
	MBDynParser& HP;
	unsigned uDriveLabel;
	mutable DriveOwner DO;

	void Check(void) const;

public:
	PostponedDriveCaller(MBDynParser& HP, unsigned uLabel);
	virtual ~PostponedDriveCaller(void);

	/* Scrive il contributo del DriveCaller al file di restart */
	virtual std::ostream& Restart(std::ostream& out) const;

	/* Copia */
	virtual DriveCaller* pCopy(void) const;

	doublereal dGet(void) const;
	doublereal dGet(const doublereal& dVar) const;

	/* this is about drives that are differentiable */
	virtual bool bIsDifferentiable(void) const;
	virtual doublereal dGetP(const doublereal& dVar) const;
};

inline doublereal
PostponedDriveCaller::dGet(void) const
{
	Check();
	return DO.dGet();
}

inline doublereal
PostponedDriveCaller::dGet(const doublereal& dVar) const
{
	Check();
	return DO.dGet(dVar);
}

inline bool
PostponedDriveCaller::bIsDifferentiable(void) const
{
	Check();
	return DO.bIsDifferentiable();
}

inline doublereal 
PostponedDriveCaller::dGetP(const doublereal& dVar) const
{
	Check();
	return DO.dGetP(dVar);
}

/* PostponedDriveCaller - end */

/*----------------------------------------------------------------------------
management of FileDriveCaller type
------------------------------------------------------------------------------

Coded by Luca Conti (May 2017)
*/

/*FileDriveCaller type reader: every content type must inherit
from this struct and implement its own Read method*/
struct FileDriveCallerTypeReader {
	virtual integer Read(const DataManager* pDM, MBDynParser& HP, FileDrive* pDrv) = 0;
	virtual ~FileDriveCallerTypeReader(void) { NO_OP; };
};

/*bag of FileDriveCaller type - every type is registered inside
of it by using SetFileDriveCallerType(...)*/
typedef std::map<std::string,FileDriveCallerTypeReader*> FileDriveCallerTypeMap;
extern FileDriveCallerTypeMap fileDriveCallerTypeMap;

/* FileDriveCaller type parsing checker: allows the parser
to understand if the next keyword is a FileDriveCaller type */
struct FileDriveCallerTypeWordSet : public HighParser::WordSet {
	virtual bool IsWord(const std::string& s) const;
};
typedef struct FileDriveCallerTypeWordSet FileDriveCallerTypeWordSet;
extern FileDriveCallerTypeWordSet fileDriveCallerTypeWordSet;

/* registration function: call it to register a new FileDriveCaller type*/
bool setFileDriveCallerType(const char *name, FileDriveCallerTypeReader *rf);

/*deallocation of all FileDriveCaller types in fileDriveCallerTypeMap, if any was added*/
void DestroyFileDriveCallerTypes(void);

#endif /* DRIVE__H */
