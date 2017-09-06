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


// vncEncoder - Object used to encode data for RFB
#include "stdafx.h"
#include "vncEncoder.h"

// Pixel format used internally when the client is palette-based & server is truecolour

static const rfbPixelFormat BGR233Format = {
    8, 8, 0, 1, 7, 7, 3, 0, 3, 6
};

// The base (RAW) encoder class

vncEncoder::vncEncoder()
{
	bufoff = 0;
	ZeroMemory(&m_remoteformat, sizeof(m_remoteformat));
	ZeroMemory(&m_localformat, sizeof(m_localformat));
	ZeroMemory(&m_transformat, sizeof(m_transformat));
	m_transtable = NULL;
	m_localpalette = NULL;
	m_bytesPerRow = 0;
	m_compresslevel = 6;
	m_qualitylevel = -1;
	m_use_lastrect = false;
}

vncEncoder::~vncEncoder()
{
	if (m_transtable != NULL)
	{
		free(m_transtable);
		m_transtable = NULL;
	}
	if (m_localpalette != NULL)
	{
		free(m_localpalette);
		m_localpalette = NULL;
	}
}

void
vncEncoder::Init()
{
	dataSize = 0;
	rectangleOverhead = 0;
	encodedSize = 0;
	transmittedSize = 0;
}

void
vncEncoder::LogStats()
{
	printf(("%s encoder stats: data=%d, overhead=%d, "
									"encoded=%d, sent=%d\n"),
				 GetEncodingName(),
				 dataSize, rectangleOverhead, encodedSize, transmittedSize);

	if (dataSize != 0) {
		printf(("%s encoder efficiency: %.3f%%\n"),
					 GetEncodingName(),
					 (double)((double)((dataSize - transmittedSize) * 100) / dataSize));
	}
}

UINT
vncEncoder::RequiredBuffSize(UINT width, UINT height)
{
	return sz_rfbFramebufferUpdateRectHeader +
		(width * height * m_remoteformat.bitsPerPixel)/8;
}

UINT
vncEncoder::NumCodedRects(RECT &rect)
{
	return 1;
}

// Translate a rectangle
inline void
vncEncoder::Translate(BYTE *source, BYTE *dest, const RECT &rect)
{
// IMPORTANT: ASSUME backbuffer-relative coordinates for rect
	// Calculate where in the source rectangle to read from
	BYTE *sourcepos = (BYTE *)(source + (m_bytesPerRow * rect.top)+(rect.left * (m_localformat.bitsPerPixel / 8)));

	// Call the translation function
	(*m_transfunc) (m_transtable,
					&m_localformat,
					&m_transformat,
					(char *)sourcepos,
					(char *)dest,
					m_bytesPerRow,
					rect.right-rect.left,
					rect.bottom-rect.top
					);
}

// Translate a rectangle (using arbitrary m_bytesPerRow value,
// always translating from the beginning of the source pixel array)
// NOTE: overloaded function!
inline void
vncEncoder::Translate(BYTE *source, BYTE *dest, int w, int h, int bytesPerRow)
{
	// Call the translation function
	(*m_transfunc) (m_transtable, &m_localformat, &m_transformat,
					(char *)source, (char *)dest, bytesPerRow, w, h);
}

// Encode a rectangle
inline UINT
vncEncoder::EncodeRect(BYTE *source, BYTE *dest, const RECT &rect, int offsetx, int offsety)
{
	const int rectW = rect.right - rect.left;
	const int rectH = rect.bottom - rect.top;

	// Create the header for the update in the destination area
	rfbFramebufferUpdateRectHeader *surh = (rfbFramebufferUpdateRectHeader *)dest;
	surh->r.x = (CARD16) (rect.left - offsetx);
	surh->r.y = (CARD16) (rect.top - offsety);
	surh->r.w = (CARD16) (rectW);
	surh->r.h = (CARD16) (rectH);
	surh->r.x = Swap16IfLE(surh->r.x);
	surh->r.y = Swap16IfLE(surh->r.y);
	surh->r.w = Swap16IfLE(surh->r.w);
	surh->r.h = Swap16IfLE(surh->r.h);
	surh->encoding = Swap32IfLE(rfbEncodingRaw);

	// Update raw encoding statistics
	rectangleOverhead += sz_rfbFramebufferUpdateRectHeader;
	dataSize += ( rectW * rectH * m_remoteformat.bitsPerPixel) / 8;
	encodedSize += ( rectW * rectH * m_remoteformat.bitsPerPixel) / 8;
	transmittedSize += sz_rfbFramebufferUpdateRectHeader + ( rectW * rectH * m_remoteformat.bitsPerPixel) / 8;

	// Translate the data in place in the output buffer
	Translate(source, dest + sz_rfbFramebufferUpdateRectHeader, rect);

	// Return the buffer size
	return sz_rfbFramebufferUpdateRectHeader +
		(rectW*rectH*m_remoteformat.bitsPerPixel) / 8;
}

// Encode a rectangle directly to the output stream.
// This implementation may not be the best, but it will work with all
// of the existing EncodeRect(BYTE *, BYTE *, const RECT &) implementations.
// Note, that the returned value is that of any data in the dest buffer that
// was not yet transmitted on the outConn.
// The primary justification for adding this method is to allow encodings to
// transmit partial data during the encoding process.  This can improve
// performance considerably for slower (more complex) encoding algorithms.
inline UINT
vncEncoder::EncodeRect(BYTE *source, char *outfile, BYTE *dest, const RECT &rect, int offsetx, int offsety)
{
	return EncodeRect(source, dest, rect, offsetx, offsety);
}

bool
vncEncoder::GetRemotePalette(RGBQUAD *quadlist, UINT ncolours)
{
	printf(("remote palette data requested\n"));

	// If the local server is palette-based then call SetTranslateFunction
	// to update the palette-to-truecolour mapping:
	if (!m_localformat.trueColour)
	{
		if (!SetTranslateFunction())
			return false;
	}

	// If the client is truecolour then don't fill in the palette buffer...
	if (m_remoteformat.trueColour)
		return false;

	// If the server is truecolour then fake BGR233
	if (m_localformat.trueColour)
	{
		// Fake BGR233...
		printf(("generating BGR233 palette data\n"));

		int ncolours = 1 << m_transformat.bitsPerPixel;
		if (m_localpalette != NULL)
			free(m_localpalette);
		m_localpalette = (char *)malloc(ncolours * sizeof(RGBQUAD));
		
		if (m_localpalette != NULL)
		{
			RGBQUAD *colour = (RGBQUAD *)m_localpalette;

			for (int i=0; i<ncolours; i++)
			{
				colour[i].rgbBlue = (((i >> m_transformat.blueShift) & m_transformat.blueMax) * 255) / m_transformat.blueMax;
				colour[i].rgbRed = (((i >> m_transformat.redShift) & m_transformat.redMax) * 255) / m_transformat.redMax;
				colour[i].rgbGreen = (((i >> m_transformat.greenShift) & m_transformat.greenMax) * 255) / m_transformat.greenMax;
			}
		}
	}
	else
	{
		// Set up RGBQUAD rfbPixelFormat info
		printf(("generating 8-bit palette data\n"));

		rfbPixelFormat remote;
		remote.trueColour = true;
		remote.bitsPerPixel = 32;
		remote.depth = 24;
		remote.bigEndian = false;
		remote.redMax = remote.greenMax = remote.blueMax = 255;
		remote.redShift = 16;
		remote.greenShift = 8;
		remote.blueShift = 0;

		// We get the ColourMapSingleTableFns procedure to handle retrieval of the
		// palette for us, to avoid replicating the code!
		(*rfbInitColourMapSingleTableFns[remote.bitsPerPixel / 16])
			(&m_localpalette, &m_localformat, &remote);
	}

	// Did we create some palette info?
	if (m_localpalette == NULL)
	{
		printf(("failed to obtain colour map data!\n"));
		return false;
	}

	// Copy the data into the RGBQUAD buffer
	memcpy(quadlist, m_localpalette, ncolours*sizeof(RGBQUAD));

	return true;
}

bool
vncEncoder::SetTranslateFunction()
{
	printf(("SetTranslateFunction called\n"));

	// By default, the actual format translated to matches the client format
	m_transformat = m_remoteformat;

    // Check that bits per pixel values are valid

    if ((m_transformat.bitsPerPixel != 8) &&
		(m_transformat.bitsPerPixel != 16) &&
		(m_transformat.bitsPerPixel != 32))
    {
		printf(("only 8, 16 or 32 bits supported remotely - %d requested\n"),
			m_transformat.bitsPerPixel
			);

		return false;
    }

    if ((m_localformat.bitsPerPixel != 8) &&
		(m_localformat.bitsPerPixel != 16) &&
		(m_localformat.bitsPerPixel != 32))
    {
		printf(("only 8, 16 or 32 bits supported locally - %d in use\n"),
			m_localformat.bitsPerPixel
			);

		return false;
    }

	if (!m_transformat.trueColour && (m_transformat.bitsPerPixel != 8))
	{
		printf(("only 8-bit palette format supported remotely\n"));
		return false;
	}
	if (!m_localformat.trueColour && (m_localformat.bitsPerPixel != 8))
	{
		printf(("only 8-bit palette format supported locally\n"));
		return false;
	}

	// Now choose the translation function to use

	// We don't do remote palettes unless they're 8-bit
    if (!m_transformat.trueColour)
	{
		// Is the local format the same?
		if (!m_localformat.trueColour &&
			(m_localformat.bitsPerPixel == m_transformat.bitsPerPixel))
		{
			// Yes, so don't do any encoding
			printf(("no encoding required - both 8-bit palettized\n"));

			m_transfunc = rfbTranslateNone;

			// The first time the client sends an update, it will call
			// GetRemotePalette to get the palette information required
			return true;
		}
		else if (m_localformat.trueColour)
		{
			// Local side is truecolour, remote is palettized
			printf(("local truecolour, remote palettized.  using BGR233 palette\n"));

			// Fill out the translation table as if writing to BGR233
			m_transformat = BGR233Format;

			// Continue on down to the main translation section
		}
		else
		{
			// No, so not supported yet...
			printf(("unknown local pixel format in use!\n"));
			return false;
		}
	}

	// REMOTE FORMAT IS TRUE-COLOUR

	// Handle 8-bit palette-based local data
	if (!m_localformat.trueColour)
	{
		// 8-bit palette to truecolour...

		// Yes, so pick the right translation function!
		printf(("using 8-bit colourmap to truecolour translation\n"));

		m_transfunc = rfbTranslateWithSingleTableFns
			[m_localformat.bitsPerPixel / 16]
			[m_transformat.bitsPerPixel / 16];

		(*rfbInitColourMapSingleTableFns[m_transformat.bitsPerPixel / 16])
			(&m_transtable, &m_localformat, &m_transformat);
		return m_transtable != NULL;
	}

	// If we reach here then we're doing truecolour to truecolour

	// Are the formats identical?
    if (PF_EQ(m_transformat,m_localformat))
	{
		// Yes, so use the null translation function
		printf(("no translation required\n"));

		m_transfunc = rfbTranslateNone;

		return true;
    }

	// Is the local display a 16-bit one
    if (m_localformat.bitsPerPixel == 16)
	{
		// Yes, so use a single lookup-table
		printf(("single LUT used\n"));

		m_transfunc = rfbTranslateWithSingleTableFns
			[m_localformat.bitsPerPixel / 16]
			[m_transformat.bitsPerPixel / 16];

		(*rfbInitTrueColourSingleTableFns[m_transformat.bitsPerPixel / 16])
			(&m_transtable, &m_localformat, &m_transformat);
    }
	else
	{
		// No, so use three tables - one for each of R, G, B.
		printf(("triple LUT used\n"));

		m_transfunc = rfbTranslateWithRGBTablesFns
			[m_localformat.bitsPerPixel / 16]
			[m_transformat.bitsPerPixel / 16];

		(*rfbInitTrueColourRGBTablesFns[m_transformat.bitsPerPixel / 16])
			(&m_transtable, &m_localformat, &m_transformat);
    }

	return m_transtable != NULL;
}

bool
vncEncoder::SetLocalFormat(rfbPixelFormat &pixformat, int width, int height)
{
	// Work out the bytes per row at the local end - useful
	m_bytesPerRow = width * pixformat.bitsPerPixel/8;

	// Save the pixel format
	m_localformat = pixformat;

	// Don't call SetTranslateFunction() if remote format is not set yet.
	if (m_remoteformat.depth == 0)
		return true;

	return SetTranslateFunction();
}

bool
vncEncoder::SetRemoteFormat(rfbPixelFormat &pixformat)
{
	// Save the client pixel format
	m_remoteformat = pixformat;

	return SetTranslateFunction();
}

void
vncEncoder::SetCompressLevel(int level)
{
	m_compresslevel = (level >= 0 && level <= 9) ? level : 6;
}

void
vncEncoder::SetQualityLevel(int level)
{
	m_qualitylevel = (level >= 0 && level <= 9) ? level : -1;
}
