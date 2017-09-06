//  Copyright (C) 2003-2006 Constantin Kaplinsky. All Rights Reserved.
//  Copyright (C) 2000 Tridia Corporation. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// TightVNC distribution homepage on the Web: http://www.tightvnc.com/
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// Many thanks to Randy Brown <rgb@inven.com> for providing the 3-button
// emulation code.

// This is the main source for a ClientConnection object.
// It handles almost everything to do with a connection to a server.
// The decoding of specific rectangle encodings is done in separate files.

#include "stdafx.h"

#include "ClientConnection.h"
#include "commctrl.h"

#define INITIALNETBUFSIZE 4096
#define MAX_ENCODINGS 20
#define VWR_WND_CLASS_NAME _T("VNCviewer")

/*
 * Macro to compare pixel formats.
 */

#define PF_EQ(x,y)							\
	((x.bitsPerPixel == y.bitsPerPixel) &&				\
	 (x.depth == y.depth) &&					\
	 ((x.bigEndian == y.bigEndian) || (x.bitsPerPixel == 8)) &&	\
	 (x.trueColour == y.trueColour) &&				\
	 (!x.trueColour || ((x.redMax == y.redMax) &&			\
			    (x.greenMax == y.greenMax) &&		\
			    (x.blueMax == y.blueMax) &&			\
			    (x.redShift == y.redShift) &&		\
			    (x.greenShift == y.greenShift) &&		\
			    (x.blueShift == y.blueShift))))

const rfbPixelFormat vnc8bitFormat = {8, 8, 0, 1, 7,7,3, 0,3,6,0,0};
const rfbPixelFormat vnc16bitFormat = {16, 16, 0, 1, 63, 31, 31, 0,6,11,0,0};



static WNDCLASS wndclass;	// FIXME!


ClientConnection::ClientConnection(HDC hdc, char* gBuffer)
{
	bufoff = 0;
	m_sock = gBuffer;
	m_hBitmapDC = hdc;
}

ClientConnection::~ClientConnection()
{
	//m_sock.close();
}
// A ScreenUpdate message has been received

void ClientConnection::ReadScreenUpdate() {

	/*rfbFramebufferUpdateMsg sut;
	ReadExact((char *) &sut, sz_rfbFramebufferUpdateMsg);
    sut.nRects = Swap16IfLE(sut.nRects);
	if (sut.nRects == 0) return;*/
	
	//for (int i=0; i < sut.nRects; i++) {

		rfbFramebufferUpdateRectHeader surh;
		ReadExact((char *) &surh, sz_rfbFramebufferUpdateRectHeader);

		surh.encoding = Swap32IfLE(surh.encoding);
		surh.r.x = Swap16IfLE(surh.r.x);
		surh.r.y = Swap16IfLE(surh.r.y);
		surh.r.w = Swap16IfLE(surh.r.w);
		surh.r.h = Swap16IfLE(surh.r.h);

		if (surh.encoding != rfbEncodingLastRect) {

			if (surh.encoding == rfbEncodingTight)
				ReadTightRect(&surh);

			// Tell the system to update a screen rectangle. Note that
			// InvalidateScreenRect member function knows about scaling.
			RECT rect;
			SetRect(&rect, surh.r.x, surh.r.y,
				surh.r.x + surh.r.w, surh.r.y + surh.r.h);
		}
	//}	

}	


// General utilities -------------------------------------------------

// Reads the number of bytes specified into the buffer given
void ClientConnection::ReadExact(char *inbuf, int wanted)
{
	int offset = 0;
    printf(("  reading %d bytes\n"), wanted);
	memset(inbuf, 0x00, wanted);
	memcpy(inbuf, m_sock + bufoff, wanted);
	bufoff += wanted;
	
}

void ClientConnection::SetPixelFormat(rfbPixelFormat &pixformat)
{
	// Save the client pixel format
	m_myFormat = pixformat;

}

// Makes sure netbuf is at least as big as the specified size.
// Note that netbuf itself may change as a result of this call.
// Throws an exception on failure.
void ClientConnection::CheckBufferSize(size_t bufsize)
{
	if (m_netbufsize > bufsize) return;

	// Don't try to allocate more than 2 gigabytes.
	if (bufsize >= 0x80000000) {
		printf(("Requested buffer size is too big (%u bytes)\n"),
			(unsigned int)bufsize);
		printf("Requested buffer size is too big.");
	}


	char *newbuf = new char[bufsize + 256];
	if (newbuf == NULL) {
		printf("Insufficient memory to allocate network buffer.");
	}

	if (m_netbuf != NULL) {
		delete[] m_netbuf;
	}
	m_netbuf = newbuf;
	m_netbufsize = bufsize + 256;
	printf(("Buffer size expanded to %u\n"),
		(unsigned int)m_netbufsize);
}

// Makes sure zlibbuf is at least as big as the specified size.
// Note that zlibbuf itself may change as a result of this call.
// Throws an exception on failure.
void ClientConnection::CheckZlibBufferSize(size_t bufsize)
{
	if (m_zlibbufsize > bufsize) return;

	// Don't try to allocate more than 2 gigabytes.
	if (bufsize >= 0x80000000) {
		printf(("Requested zlib buffer size is too big (%u bytes)\n"),
			(unsigned int)bufsize);
		printf("Requested zlib buffer size is too big.");
	}

	// omni_mutex_lock l(m_bufferMutex);

	unsigned char *newbuf = new unsigned char[bufsize + 256];
	if (newbuf == NULL) {
		printf("Insufficient memory to allocate zlib buffer.");
	}

	if (m_zlibbuf != NULL) {
		delete[] m_zlibbuf;
	}
	m_zlibbuf = newbuf;
	m_zlibbufsize = bufsize + 256;
	printf(("Zlib buffer size expanded to %u\n"),
		(unsigned int)m_zlibbufsize);
}