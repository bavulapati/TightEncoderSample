//  Copyright (C) 2001 Constantin Kaplinsky. All Rights Reserved.
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


// vncEncoder object

// The vncEncoder object encodes regions of a display buffer for sending
// to a client

class vncEncoder;

#if !defined(RFBENCODER_DEFINED)
#define RFBENCODER_DEFINED
#pragma once


#include "translate.h"
#include <stdbool.h>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <Windows.h>
#include <fstream>

//typedef unsigned int UINT;
//typedef unsigned char BYTE;
//
//typedef struct tagRECT {
//	long left;
//	long top;
//	long right;
//	long bottom;
//} RECT;
//typedef struct tagRGBQUAD {
//	BYTE rgbBlue;
//	BYTE rgbGreen;
//	BYTE rgbRed;
//	BYTE rgbReserved;
//} RGBQUAD;

// Class definition

class vncEncoder
{
// Fields
public:
	int bufoff;
// Methods
public:
	// Create/Destroy methods
	vncEncoder();
	virtual ~vncEncoder();

	// Initialisation
	virtual void Init();

	// A method to return the encoding name, used by the LogStats() method
	virtual const char* GetEncodingName() { return "Raw"; }

	// Central method for outputing encoding statistics
	virtual void LogStats();

	// Encoder stats used by the buffer object
	virtual UINT RequiredBuffSize(UINT width, UINT height);
	virtual UINT NumCodedRects(RECT &rect);

	// Translation & encoding routines
	//  - Source is the base address of the ENTIRE SCREEN buffer.
	//    The Translate routine compensates automatically for the desired rectangle.
	//  - Dest is the base address to encode the rect to.  The rect will be encoded
	//    into a contiguous region of the buffer.
	virtual void Translate(BYTE *source, BYTE *dest, const RECT &rect);
	virtual UINT EncodeRect(BYTE *source, BYTE *dest, const RECT &rect, int offsetx, int offsety);
	virtual UINT EncodeRect(BYTE *source, char *outfile, BYTE *dest, const RECT &rect, int offsetx, int offsety);

	// Additional translation function for cursor shape data (overloaded!)
	void Translate(BYTE *source, BYTE *dest, int w, int h, int bytesPerRow);

	// Translation handling
	bool SetLocalFormat(rfbPixelFormat &pixformat, int width, int height);
	bool SetRemoteFormat(rfbPixelFormat &pixformat);

	// Configuring encoder
	void SetCompressLevel(int level);
	void SetQualityLevel(int level);
	void EnableLastRect(bool enable) { m_use_lastrect = enable; }

	// Colour map handling
	bool GetRemotePalette(RGBQUAD *quadlist, UINT ncolours);


protected:
	bool SetTranslateFunction();

// Implementation
protected:
	rfbTranslateFnType	m_transfunc;			// Translator function
	char*				m_transtable;			// Colour translation LUT
	char*				m_localpalette;			// Palette info if client is palette-based
	rfbPixelFormat		m_localformat;			// Pixel Format info
	rfbPixelFormat		m_remoteformat;			// Client pixel format info
	rfbPixelFormat		m_transformat;			// Internal format used for translation (usually == client format)
	int					m_bytesPerRow;			// Number of bytes per row locally
	int					dataSize;				// Total size of raw data encoded
	int					rectangleOverhead;		// Total size of rectangle header data
	int					encodedSize;			// Total size of encoded data
	int					transmittedSize;		// Total amount of data sent

	int					m_compresslevel;		// Encoding-specific compression level (if needed).
	int					m_qualitylevel;			// Image quality level for lossy JPEG compression.
	bool				m_use_lastrect;			// LastRect pseudo-encoding allowed.
};

#endif // vncENCODER_DEFINED