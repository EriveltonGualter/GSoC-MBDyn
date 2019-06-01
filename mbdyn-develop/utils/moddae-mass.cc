/* $Header$ */
/*
 * MBDyn (C) is a multibody analysis code.
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2017
 *
 * Pierangelo Masarati  <masarati@aero.polimi.it>
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

#include <fstream>
#include <iostream>
#include <cmath>

#include "myassert.h"
#include "solman.h"
#include "dae-intg.h"

struct private_data {
	doublereal m1;
	doublereal m2;
	doublereal k;
	doublereal f;
	doublereal omega;
	doublereal x[5];
	doublereal xP[5];
};

static int
read(void** pp, const char* user_defined)
{
	*pp = (void*)new private_data;
	private_data* pd = (private_data*)*pp;

	if (user_defined != NULL) {
		std::ifstream in(user_defined);
		if (!in) {
			std::cerr << "unable to open file "
				"\"" << user_defined << "\"" << std::endl;
			exit(EXIT_FAILURE);
		}

		in >> pd->m1 >> pd->m2 >> pd->k
			>> pd->f >> pd->omega;

	} else {
		pd->m1 = 1.;
		pd->m2 = 1.;
		pd->k = 1.;
		pd->f = 1.;
		pd->omega = 1.;
	}

	doublereal f = 1.;//std::sin(pd->omega*0.);
	pd->x[0] = 0.;
	pd->x[1] = 0.;
	pd->x[2] = 0.;
	pd->x[3] = 0.;
	pd->x[4] = 0.;
	pd->xP[0] = 0.;
	pd->xP[1] = 0.;
	pd->xP[2] = f/pd->m1;
	pd->xP[3] = 0.;
	pd->xP[4] = 0.;

	if (fabs(pd->x[0]-pd->x[1]) > 1.e-9) {
		std::cerr << "constraint violated" << std::endl;
		exit(EXIT_FAILURE);
	}

	doublereal lambda = pd->k*(pd->x[3]-pd->x[2]);
	if (fabs(lambda - pd->xP[4]) > 1.e-9) {
		std::cerr << "constraint reaction incorrect" << std::endl;
		pd->xP[4] = lambda;
	}

	return 0;
}

static int
size(void* p)
{
	return 5;
}

static int
init(void* p, VectorHandler& X, VectorHandler& XP)
{
	private_data* pd = (private_data*)p;

	X.Reset();
	XP.Reset();

	for (int i = 1; i <= size(p); i++) {
		/* velocita' iniziale */
		XP(i) = pd->xP[i-1];
		/* posizione iniziale */
		X(i) = pd->x[i-1];
	}

	return 0;
}

static int
grad(void* p, MatrixHandler& J, MatrixHandler& JP,
		const VectorHandler& X, const VectorHandler& XP,
		const doublereal& t)
{
	private_data* pd = (private_data*)p;

	doublereal m1 = pd->m1;
	doublereal m2 = pd->m2;
	doublereal k = pd->k;

	J(1, 3) = 1.;
	J(2, 4) = 1.;
	J(3, 1) = -k;
	J(4, 2) = -k;
	J(5, 1) = -1.;
	J(5, 2) = 1.;

	JP(1, 1) = -1.;
	JP(2, 2) = -1.;
	JP(3, 3) = -m1;
	JP(3, 5) = 1;
	JP(4, 4) = -m2;
	JP(4, 5) = -1;

	return 0;
}

static int
func(void* p, VectorHandler& R,
		const VectorHandler& X, const VectorHandler& XP,
		const doublereal& t)
{
	private_data* pd = (private_data*)p;

	doublereal x = X(1);
	doublereal y = X(2);
	doublereal u = X(3);
	doublereal v = X(4);
	doublereal xP = XP(1);
	doublereal yP = XP(2);
	doublereal uP = XP(3);
	doublereal vP = XP(4);
	doublereal lambda = XP(5);

	doublereal m1 = pd->m1;
	doublereal m2 = pd->m2;
	doublereal k = pd->k;
	doublereal f = 1.;

	R(1) = xP - u;
	R(2) = yP - v;
	R(3) = m1*uP + k*x - lambda - f;
	R(4) = m2*vP + k*y + lambda;
	R(5) = x - y;

	return 0;
}

static std::ostream&
out(void* p, std::ostream& o, const VectorHandler& X, const VectorHandler& XP)
{
	private_data* pd = (private_data*)p;

	doublereal x = X(1);
	doublereal y = X(2);
	doublereal xP = X(3);
	doublereal yP = X(4);
	doublereal lambda = XP(5);

	doublereal E = .5*pd->k*(x*x+y*y);

	return o
		<< " " << XP(1)		/*  3 */
		<< " " << XP(2)		/*  4 */
		<< " " << x 		/*  5 */
		<< " " << y 		/*  6 */
		<< " " << xP		/*  7 */
		<< " " << yP		/*  8 */
		<< " " << E		/*  9 */
		<< " " << lambda;	/* 10 */
}

static int
destroy(void** p)
{
	private_data* pd = (private_data*)(*p);

	delete pd;
	*p = NULL;

	return 0;
}

static struct funcs funcs_handler = {
	read,
	0,
	init,
	size,
	grad,
	func,
	out,
	destroy
};

/* de-mangle name */
extern "C" {
void *ff = &funcs_handler;
}

