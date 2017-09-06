// TightEncodingPOC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include "vncEncodeTight.h"

typedef struct {		//modified for test case
	HWND hWnd;
	HDC hDoubleBuffer;
	HDC hDoubleBufferWorker;
	BITMAPINFO BitmapInfo;
	HBITMAP hCompatBitmap;
	HBITMAP hOldCompatBitmap;
	HGDIOBJ hOldBitMap;
	HBITMAP hDBBitMap;
	UCHAR *pBuffer;
	/*UCHAR *pBuffer1;
	UCHAR *pBuffer2;
	UCHAR *pBuffer3;
	UCHAR *pBuffer4;*/
	DWORD dwScreenX;
	DWORD dwScreenY;
	PALETTEENTRY VirtualPalette[256];
	DWORD dwOptimizedPaletteLookUp[256];
	CRITICAL_SECTION csRepaintBlock;
}GDI, *PGDI;

int main()
{
	// Application name
	const char	*szAppName = "TightEnc";

	vncEncoder	   *m_encoder = new vncEncodeTight;

	rfbPixelFormat		m_clientformat = { 8, 8, 0, 1, 7, 7, 3, 0, 3, 6 };
	rfbServerInitMsg	m_scrinfo = {1366, 728, m_clientformat, 8};
	
	// These variables mirror similar variables from vncEncoder class.
	// They are necessary because vncEncoder instance may be created after
	// their values were set.
	int				m_compresslevel = 6;
	int				m_qualitylevel = 5;
	bool			m_use_lastrect = false;

	// Initialise it and give it the pixel format
	m_encoder->Init();
	m_encoder->SetLocalFormat(
		m_scrinfo.format,
		m_scrinfo.framebufferWidth,
		m_scrinfo.framebufferHeight);

	// Duplicate our member fields in new Encoder.
	m_encoder->SetCompressLevel(m_compresslevel);
	m_encoder->SetQualityLevel(m_qualitylevel);
	m_encoder->EnableLastRect(m_use_lastrect);

	if (!m_encoder->SetRemoteFormat(m_clientformat))
	{
		printf(("client pixel format is not supported\n"));
	}

	RECT rect = { 0, 0, 1365, 767 };
	BYTE *scrBuff;

	// device contexts for memory and the screen
	HDC				m_hmemdc;
	HDC				m_hrootdc;

	m_hrootdc = ::GetDC(NULL); 
	// Create a compatible memory DC
	m_hmemdc = CreateCompatibleDC(m_hrootdc);

	// Capture screen into bitmap
	BOOL blitok = BitBlt(
		m_hmemdc,
		// source in m_hrootdc is relative to a virtual desktop,
		// whereas dst coordinates of m_hmemdc are relative to its top-left corner (0, 0)
		rect.left,
		rect.top,
		rect.right,
		rect.bottom,
		m_hrootdc,
		rect.left, rect.top,
		SRCCOPY);

	
	
	const int bytesPerPixel = m_scrinfo.format.bitsPerPixel / 8;
	int m_bytesPerRow = m_scrinfo.framebufferWidth * m_scrinfo.format.bitsPerPixel / 8;
	BYTE *destbuff = (BYTE*)malloc(m_scrinfo.framebufferWidth*m_scrinfo.framebufferHeight * 3);

	HBITMAP bmparr[2];
	BITMAP bitmapObject;
	bmparr[0] = (HBITMAP)LoadImage(GetModuleHandle(NULL), (LPCWSTR)/*"../encoderTest.bmp"*/"C:\\Users\\balakrishna\\Desktop\\New folder\\encoderTest.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE);


	GetObject(bmparr[0], sizeof(BITMAP), &bitmapObject);

	BITMAPINFO *bmi = (BITMAPINFO *)_alloca(sizeof(BITMAPINFOHEADER)
		+ 256 * sizeof(RGBQUAD));

	memset(&bmi->bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	int scanLineCount = GetDIBits(m_hrootdc, (HBITMAP)bmparr[0], 0, bitmapObject.bmHeight,
		NULL, bmi, DIB_RGB_COLORS);

	int imageBytes = m_scrinfo.framebufferWidth*m_scrinfo.framebufferHeight * 3;
	scrBuff = (BYTE *)malloc(imageBytes);
	scanLineCount = GetDIBits(m_hrootdc, (HBITMAP)bmparr[0], 0, bitmapObject.bmHeight,
		scrBuff, bmi, DIB_RGB_COLORS);

	/*pImg.Size.cx = bitmapObject.bmWidth;
	pImg.Size.cy = bitmapObject.bmHeight;
	pImg.DPI.cx = 200;
	pImg.DPI.cy = 200;
	pImg.BytesPerLine = bitmapObject.bmWidthBytes;
	pImg.BitsPerPixel = bitmapObject.bmBitsPixel;*/
	
	std::ofstream output_file("students.data", std::iostream::binary);

	m_encoder->EncodeRect(scrBuff, destbuff, rect, 0, 0);
	
	output_file.close();


	printf("%s", szAppName);

    return 0;
}

//void get() {
//	HDC hDisplayDC = CreateDC((LPCWSTR)"DISPLAY", NULL, NULL, NULL);
//	bool ret = 0;
//
//	PGDI m_pGDI;
//
//	ret = BitBlt(m_pGDI->hDoubleBufferWorker, 0, 0, 1366,
//		768, hDisplayDC, 0, 0, SRCCOPY | CAPTUREBLT);
//}

//void CopyToBuffer(rfbServerInitMsg	m_scrinfo, RECT rect, BYTE *destbuff, const BYTE *srcbuffpos)
//{
//
//	const int bytesPerPixel = m_scrinfo.format.bitsPerPixel / 8;
//	
//	// Calculate the number of bytes per row
//	int m_bytesPerRow = m_scrinfo.framebufferWidth * m_scrinfo.format.bitsPerPixel / 8;
//
//	BYTE *destbuffpos = destbuff;
//
//	const int widthBytes = (rect.right - rect.left) * bytesPerPixel;
//
//	for (int y = rect.top; y < rect.bottom; y++)
//	{
//		memcpy(destbuffpos, srcbuffpos, widthBytes);
//		srcbuffpos += m_bytesPerRow;
//		destbuffpos += m_bytesPerRow;
//	}
//}