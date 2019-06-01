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
 * Michele Attolico <attolico@aero.polimi.it>
 */

/*
 * (Portions)
 *
 * AUTHOR: Dr. Rudolf Jaeger <rudijaeger@yahoo.com>
 * Copyright (C) 2008 all rights reserved.
 *
 * The copyright of this patch is trasferred
 * to Pierangelo Masarati and Paolo Mantegazza
 * for use in the software MBDyn as described 
 * in the GNU Public License version 2.1
 */

#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */

#include "dataman.h"
#include "filedrv.h"
#include "streamdrive.h"
#include "socketstreamdrive.h"

#ifdef USE_SOCKET

#include "sock.h"

#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include "rtai_in_drive.h"

#define DEFAULT_PORT	9012	/* intentionally unassigned by IANA */
#define DEFAULT_HOST	"127.0.0.1"

SocketStreamDrive::SocketStreamDrive(unsigned int uL,
	const DriveHandler* pDH,
	UseSocket *pUS, bool c,
	const std::string& sFileName,
	integer nd, const std::vector<doublereal>& v0,
	StreamDrive::Modifier *pMod,
	unsigned int ie, bool bReceiveFirst,
	int flags,
	const struct timeval& st,
	StreamDriveEcho *pSDE)
: StreamDrive(uL, pDH, sFileName, nd, v0, c, pMod),
InputEvery(ie), bReceiveFirst(bReceiveFirst), InputCounter(ie - 1),
pUS(pUS), recv_flags(flags),
SocketTimeout(st),
pSDE(pSDE)
{
	// NOTE: InputCounter is set to InputEvery - 1 so that input
	// is expected at initialization (initial time) and then every
	// InputEvery steps; for example, for InputEvery == 4, input
	// is expected at:
	//	initial time
	//	initial time + 4 * timestep
	//	initial time + 8 * timestep
	ASSERT(InputEvery > 0);

	if (!bReceiveFirst) {
		InputCounter -= InputEvery;
	}

	if (pSDE) {
		pSDE->Init("SocketStreamDrive", uLabel, nd);
	}
}

SocketStreamDrive::~SocketStreamDrive(void)
{
	if (pUS != 0) {
		SAFEDELETE(pUS);
	}

	if (pSDE != 0) {
		delete pSDE;
	}
}

/* Scrive il contributo del DriveCaller al file di restart */   
std::ostream&
SocketStreamDrive::Restart(std::ostream& out) const
{
	out << "  file: " << uLabel << ", socket stream," 
		" stream drive name, \"" << sFileName << "\"";
	pUS->Restart(out);
	return out << ", " << iNumDrives << ";" << std::endl;
}
   
void
SocketStreamDrive::ServePending(const doublereal& t)
{
	
	// by now, an abandoned drive is not read any more;
	// should we retry or what?
	if (pUS->Abandoned()) {
		silent_cout("SocketStreamDrive(" << sFileName << "): "
			"abandoned"  << std::endl);
		return;
	}

	ASSERT(pUS->Connected());
	
	/* read only every InputEvery steps */
	InputCounter++;
	if (InputCounter != InputEvery) {
		return;
	}
	InputCounter = 0;
	
	int sock_nr = pUS->GetSock();
	ssize_t rc = -1;
	// Use socket timeout if set in input file; Default: 0
	if (SocketTimeout.tv_sec || SocketTimeout.tv_usec) {
		// Use Select() on the socket for automatic shutdown if
		// socket clients fail.
		fd_set readfds;

		// Clear the set
		FD_ZERO(&readfds);

		// Add descriptors to the set
		FD_SET(sock_nr, &readfds);

		// Copy timeout because select(2) may overwrite it
		struct timeval tv = SocketTimeout;

		// Call select
		rc = select(sock_nr + 1, &readfds, NULL, NULL, &tv);
		switch (rc) {
		case -1: {
			int save_errno = errno;
			char *err_msg = strerror(save_errno);

			silent_cout("SocketStreamDrive"
				"(" << sFileName << "): select failed"
				<< " (" << save_errno << ": " 
				<< err_msg << ")" << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
			}

		case 0:
			silent_cout("SocketStreamDrive"
				"(" << sFileName << "): select timed out"
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);

		default:
			if (!FD_ISSET(sock_nr, &readfds)) {
				silent_cout("SocketStreamDrive"
					"(" << sFileName << "): "
					"socket " << sock_nr << " reset"
					<< std::endl);
				throw ErrGeneric(MBDYN_EXCEPT_ARGS);
			}
		}
	}

	// Read data
	// NOTE: flags __SHOULD__ contain MSG_WAITALL;
	// however, it is not defined on some platforms (e.g. Cygwin)
	// TODO: needs work for network independence!
	rc = pUS->recv(&buf[0], size, recv_flags);

	/* FIXME: no receive at first step? */
	switch (rc) {
	case 0:
do_abandon:;
		silent_cout("SocketStreamDrive(" << sFileName << "): "
			<< "communication closed by host; abandoning..."
			<< std::endl);
		pUS->Abandon();
		break;

	case -1: {
		int save_errno = errno;

		// some errno values may be legal
		switch (save_errno) {
		case EAGAIN:
			if (recv_flags & MSG_DONTWAIT) {
				// non-blocking
				return;
			}
			break;

		case ECONNRESET:
			goto do_abandon;
		}

		char *err_msg = strerror(save_errno);
		silent_cout("SocketStreamDrive(" << sFileName << ") failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}

	default:
		if (pSDE) {
			pSDE->EchoPrepare(&pdVal[1], iNumDrives);
		}

		// copy values from buffer
		pMod->Modify(&pdVal[1], &buf[0]);

		if (pSDE) {
			pSDE->Echo(&pdVal[1], iNumDrives);
		}
		break;
	}
}


/* legge i drivers tipo stream */

static Drive *
ReadStreamDrive(const DataManager *pDM, MBDynParser& HP, unsigned uLabel)
{
	bool bGotCreate(false);
	bool bCreate(false);
	unsigned short int port = -1; 
	std::string name;
	std::string host;
	std::string path;

	if (HP.IsKeyWord("name") || HP.IsKeyWord("stream" "drive" "name")) {
		const char *m = HP.GetStringWithDelims();
		if (m == 0) {
			silent_cerr("SocketStreamDrive(" << uLabel << "): "
				"unable to read stream drive name "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);

		} 

		name = m;

	} else {
		silent_cerr("SocketStreamDrive(" << uLabel << "):"
			"missing stream drive name "
			"at line " << HP.GetLineData()
			<< std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	if (HP.IsKeyWord("create")) {
		bGotCreate = true;
		if (!HP.GetYesNo(bCreate)) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"\"create\" must be either \"yes\" or \"no\" "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}

	if (HP.IsKeyWord("local") || HP.IsKeyWord("path")) {
		const char *m = HP.GetFileName();
		
		if (m == 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"unable to read local path "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		
		path = m;
	}
	
	if (HP.IsKeyWord("port")) {
		if (!path.empty()) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"cannot specify port "
				"for a local socket "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);		
		}

		int p = HP.GetInt();
		/* Da sistemare da qui */
#ifdef IPPORT_USERRESERVED
		if (p <= IPPORT_USERRESERVED) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"cannot listen on reserved port "
				<< port << ": less than "
				"IPPORT_USERRESERVED=" << IPPORT_USERRESERVED
				<< " at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		/* if #undef'd, don't bother checking;
		 * the OS will do it for us */
#endif /* IPPORT_USERRESERVED */

		port = p;
	}

	
	if (HP.IsKeyWord("host")) {
		if (!path.empty()) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"cannot specify host for a local socket "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);		
		}

		const char *h;
		
		h = HP.GetStringWithDelims();
		if (h == 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"unable to read host "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}

		host = h;

	} else if (path.empty() && !bCreate) {
		silent_cerr("SocketStreamDrive"
			"(" << uLabel << ", \"" << name << "\"): "
			"host undefined, "
			"using default \"" << DEFAULT_HOST "\" "
			"at line " << HP.GetLineData()
			<< std::endl);
		host = DEFAULT_HOST;
	}

	int socket_type = SOCK_STREAM;
	if (HP.IsKeyWord("socket" "type")) {
		if (HP.IsKeyWord("udp")) {
			socket_type = SOCK_DGRAM;
			if (!bGotCreate) {
				bCreate = true;
			}

		} else if (!HP.IsKeyWord("tcp")) {
			silent_cerr("SocketStreamDrive(" << uLabel << ", \"" << name << "\"): "
				"invalid socket type "
				"at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}

	if ((socket_type == SOCK_DGRAM) && !bCreate) {
		silent_cerr("SocketStreamDrive(" << uLabel << ", \"" << name << "\"): "
			"socket type=udp incompatible with create=no "
			"at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	// we want to block until the whole chunk is received
	int flags = 0;
#ifdef MSG_WAITALL
	flags |= MSG_WAITALL;
#endif // MSG_WAITALL

	while (HP.IsArg()) {
		if (HP.IsKeyWord("signal")) {
#ifdef MSG_NOSIGNAL
			flags &= ~MSG_NOSIGNAL;
#else // ! MSG_NOSIGNAL
			silent_cout("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"MSG_NOSIGNAL not defined (ignored) "
				"at line " << HP.GetLineData()
				<< std::endl);
#endif // ! MSG_NOSIGNAL

		// not honored by recv(2)
		} else if (HP.IsKeyWord("no" "signal")) {
#ifdef MSG_NOSIGNAL
			flags |= MSG_NOSIGNAL;
#else // ! MSG_NOSIGNAL
			silent_cout("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"MSG_NOSIGNAL not defined (ignored) "
				"at line " << HP.GetLineData()
				<< std::endl);
#endif // ! MSG_NOSIGNAL

		} else if (HP.IsKeyWord("blocking")) {
			// not honored by recv(2)?
			flags |= MSG_WAITALL;
			flags &= ~MSG_DONTWAIT;

		} else if (HP.IsKeyWord("non" "blocking")) {
			// not honored by recv(2)?
			flags &= ~MSG_WAITALL;
			flags |= MSG_DONTWAIT;

		} else {
			break;
		}
	}

	unsigned int InputEvery = 1;
	if (HP.IsKeyWord("input" "every")) {
		int i = HP.GetInt();
		if (i <= 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"invalid \"input every\" value " << i
				<< " at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		InputEvery = (unsigned int)i;
	}

	bool bReceiveFirst(true);
	if (HP.IsKeyWord("receive" "first")) {
		if (!HP.GetYesNo(bReceiveFirst)) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"\"receive first\" must be either \"yes\" or \"no\" "
				<< "at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}

	struct timeval SocketTimeout = { 0, 0 };
	if (HP.IsKeyWord("timeout")) {
		doublereal st = HP.GetReal();
		if (st < 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"invalid socket timeout value " << st
				<< " at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		SocketTimeout.tv_sec = long(st);
		SocketTimeout.tv_usec = long((st - SocketTimeout.tv_sec)*1000000);
	}

	pedantic_cout("SocketStreamDrive"
		"(" << uLabel << ", \"" << name << "\"): "
		"timeout: " << SocketTimeout.tv_sec << "s "
		<< SocketTimeout.tv_usec << "ns" << std::endl);

	StreamDriveEcho *pSDE = ReadStreamDriveEcho(pDM, HP);

	/* Luca Conti edits - GSOC 2017 */

	std::vector<doublereal> v0;
	StreamDrive::Modifier *pMod(0);
	int idrives;

	const char *s = HP.IsWord(fileDriveContentTypeWordSet);
	if (s != NULL) {
		FileDriveContentTypeMap::iterator it = fileDriveContentTypeMap.find(std::string(s));
		pMod = it->second->Read(v0, HP, idrives);
	} else {
		idrives = HP.GetInt();
		if (idrives <= 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"illegal number of channels " << idrives
				<< " at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}

		if (HP.IsKeyWord("initial" "values")) {
			v0.resize(idrives);
			for (int i = 0; i < idrives; i++) {
				v0[i] = HP.GetReal();
			}
		}

		if (HP.IsKeyWord("modifier")) {
			pMod = ReadStreamDriveModifier(HP, idrives);
		}
	}

	UseSocket *pUS = 0;


	// .log file output
	std::ostream& out = pDM->GetLogFile();
	out << "filedriver: " << uLabel << " stream";

	if (path.empty()) {
		if (port == (unsigned short int)(-1)) {
			port = DEFAULT_PORT;
		}
		SAFENEWWITHCONSTRUCTOR(pUS, UseInetSocket, UseInetSocket(host, port, socket_type, bCreate));
	
		// .log file output
		out 
			<< " INET"
			<< " " << name
			<< " " << host 
			<< " " << port;

	} else {
		SAFENEWWITHCONSTRUCTOR(pUS, UseLocalSocket, UseLocalSocket(path, socket_type, bCreate));
		out
			<< " UNIX"
			<< " " << name
			<< " " << path;
	}
	
	if (socket_type == SOCK_STREAM) {
		out << " tcp";
	} else {
		out << " udp";
	}
			
	out << " " << bCreate;


	if ((socket_type == SOCK_STREAM) && bCreate) {
		const_cast<DataManager *>(pDM)->RegisterSocketUser(pUS);

	} else {
		pUS->Connect();
	}

	Drive* pDr = 0;
	SAFENEWWITHCONSTRUCTOR(pDr, SocketStreamDrive,
		SocketStreamDrive(uLabel,
			pDM->pGetDrvHdl(), pUS, bCreate,
			name, idrives, v0, pMod,
			InputEvery, bReceiveFirst,
			flags, SocketTimeout,
			pSDE));
#ifdef MSG_NOSIGNAL
	if (flags & ~MSG_NOSIGNAL) {
		out << " " << true;
	} else {
#endif // MSG_NOSIGNAL
		out << " " << false;
#ifdef MSG_NOSIGNAL
	}
#endif

#ifdef MSG_DONTWAIT
	if (flags & MSG_DONTWAIT) {
		out << " " << true;
	} else {
#endif // MSG_DONTWAIT
		out << " " << false;
#ifdef MSG_DONTWAIT
	}
#endif

	out 
		<< " " << InputEvery
		<< " " << bReceiveFirst
		<< " " << SocketTimeout.tv_sec
		<< " " << idrives;

	for (std::vector<doublereal>::iterator it = v0.begin(); it != v0.end(); ++it)
	{
		out << " " << (*it);
	}

	out << std::endl;

	return pDr;
}

#endif // USE_SOCKET


Drive *
StreamDR::Read(unsigned uLabel, const DataManager *pDM, MBDynParser& HP)
{
	Drive *pDr = 0;

	if (!s.empty()) {
		pedantic_cout("\"" << s << "\" is deprecated; "
			"use \"stream\" instead at line "
			<< HP.GetLineData() << std::endl);
	}

#ifdef USE_RTAI
	if (::rtmbdyn_rtai_task != NULL){
		silent_cout("starting RTMBDyn drive " << uLabel << std::endl);
		pDr = ReadRTMBDynInDrive(pDM, HP, uLabel);
	} else 
#endif /* USE_RTAI */		
	{
#ifdef USE_SOCKET
		silent_cout("starting stream drive " << uLabel << std::endl);
		pDr = ReadStreamDrive(pDM, HP, uLabel);
#else // ! USE_SOCKET
		silent_cerr("stream drive " << uLabel
			<< " not allowed at line " << HP.GetLineData()
			<< " because apparently the current architecture "
			"does not support sockets" << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
#endif // ! USE_SOCKET
	}

	return pDr;
}

/* Luca Conti edits - GSOC 2017 */

FileDriveContentTypeMap fileDriveContentTypeMap;
FileDriveContentTypeWordSetType fileDriveContentTypeWordSet;

/* file drive content type parsing checker: allows the parser
to understand if the next keyword is a content type */
bool FileDriveContentTypeWordSetType::IsWord(const std::string& s) const {
		return fileDriveContentTypeMap.find(std::string(s)) != fileDriveContentTypeMap.end();
	};

/* registration function: call it to register a new content type*/
bool SetFileDriveContentType(const char *name, FileDriveContentTypeReader *rf) {
	pedantic_cout("registering file drive content type \"" << name << "\""
		<< std::endl );
	return fileDriveContentTypeMap.insert(FileDriveContentTypeMap::value_type(name, rf)).second;
}

/*deallocation of all content types in fileDriveContentTypeMap, if any was added*/
void DestroyFileDriveContentTypes(void) {
	for (FileDriveContentTypeMap::iterator i = fileDriveContentTypeMap.begin(); i != fileDriveContentTypeMap.end(); ++i) {
		delete i->second;
	}
	fileDriveContentTypeMap.clear();
}
