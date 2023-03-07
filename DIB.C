/******************************************************************************/
/* dib.c -- code to read and display .BMP files.                              */
/******************************************************************************/

#define NOKANJI
#define NOMDI
#define NOSOUND
#define NOLOGERROR
#define NOPROFILER
#define NOMETAFILE
#define NOSYSTEMPARAMSINFO
#define NOWH
#define NONLS
#define NOSERVICE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

#ifdef WIN32
#define huge /* huge */
#endif /* WIN32 */

#include "dib.h"

/* Functions for extracting information from Dib memory blocks */

DWORD GetDibInfoHeaderSize (BYTE huge * lpDib)
{
    return (((BITMAPINFOHEADER huge *) lpDib)->biSize);
}

WORD GetDibWidth (BYTE huge * lpDib)
{
    if (GetDibInfoHeaderSize (lpDib) == sizeof (BITMAPCOREHEADER))
        return ((WORD) (((BITMAPCOREHEADER huge *) lpDib)->bcWidth));
    else
        return ((WORD) (((BITMAPINFOHEADER huge *) lpDib)->biWidth));
}

WORD GetDibHeight (BYTE huge * lpDib)
{
    if (GetDibInfoHeaderSize (lpDib) == sizeof (BITMAPCOREHEADER))
        return ((WORD) (((BITMAPCOREHEADER huge *) lpDib)->bcHeight));
    else
        return ((WORD) (((BITMAPINFOHEADER huge *) lpDib)->biHeight));
}

BYTE huge * GetDibBitsAddr (BYTE huge * lpDib)
{
    DWORD dwNumColors, dwColorTableSize;
    WORD wBitCount;

    if (GetDibInfoHeaderSize (lpDib) == sizeof (BITMAPCOREHEADER))
    {
        wBitCount = ((BITMAPCOREHEADER huge *) lpDib)->bcBitCount;

        if (wBitCount != 24)
            dwNumColors = 1L << wBitCount;
        else
            dwNumColors = 0;

        dwColorTableSize = dwNumColors * sizeof (RGBTRIPLE);
    }
    else
    {
        wBitCount = ((BITMAPINFOHEADER huge *) lpDib)->biBitCount;

        if (GetDibInfoHeaderSize (lpDib) >= 36)
            dwNumColors = ((BITMAPINFOHEADER huge *) lpDib)->biClrUsed;
        else
            dwNumColors = 0;

        if (dwNumColors == 0)
        {
            if (wBitCount != 24)
                dwNumColors = 1L << wBitCount;
            else
                dwNumColors = 0;
        }
        dwColorTableSize = dwNumColors * sizeof (RGBQUAD);
    }
    return (lpDib + GetDibInfoHeaderSize (lpDib) + dwColorTableSize);
}

/* Read a Dib from a file into memory */

BYTE huge * ReadDib (LPSTR szFileName)
{
    BITMAPFILEHEADER bmfh;
    BYTE huge * lpDib;
    DWORD dwDibSize, dwOffset, dwHeaderSize;
    int hFile;
    WORD wDibRead;

    if (-1 == (hFile = _lopen (szFileName, OF_READ | OF_SHARE_DENY_WRITE)))
        return(NULL);

    if (_lread (hFile, (LPSTR) &bmfh, sizeof (BITMAPFILEHEADER)) !=
        sizeof (BITMAPFILEHEADER))
    {
        _lclose (hFile);
        return NULL;
    }

    if (bmfh.bfType != * (WORD *) "BM")
    {
        _lclose (hFile);
        return NULL;
    }

    dwDibSize = bmfh.bfSize - sizeof (BITMAPFILEHEADER);

    lpDib = (BYTE huge * ) GlobalAllocPtr (GMEM_MOVEABLE, dwDibSize);

    if (lpDib == NULL)
    {
        _lclose (hFile);
        return (NULL);
    }

    dwOffset = 0;

    while (dwDibSize > 0)
    {
        wDibRead = (WORD) min(32768ul, dwDibSize);

        if (wDibRead != _lread (hFile, (LPSTR) (lpDib + dwOffset), wDibRead))
        {
            _lclose(hFile);
            FreeDib(&lpDib);
            return (NULL);
        }

        dwDibSize -= wDibRead;
        dwOffset += wDibRead;
    }

    _lclose (hFile);

    dwHeaderSize = GetDibInfoHeaderSize (lpDib);

    if (dwHeaderSize < 12 || (dwHeaderSize > 12 && dwHeaderSize < 16))
    {
        FreeDib(&lpDib);
        return(NULL);
    }
    return (lpDib);
}

void FreeDib(BYTE huge **lplpDib)
{
    if (*lplpDib != NULL)
    {
        GlobalFreePtr(*lplpDib);
        *lplpDib = NULL;
    }
} /* FreeDib */

void PaintDibInWindow(BYTE huge * lpDib, HDC hdc)
{
    short cxDib, cyDib;
    BYTE huge * lpDibBits;

    if (lpDib != NULL)
    {
        lpDibBits = GetDibBitsAddr (lpDib);
        cxDib = GetDibWidth (lpDib);
        cyDib = GetDibHeight (lpDib);

        SetStretchBltMode (hdc, COLORONCOLOR);

        SetDIBitsToDevice (hdc, 0, 0, cxDib, cyDib, 0, 0,
                           0, cyDib, (LPSTR) lpDibBits,
                           (LPBITMAPINFO) lpDib,
                           DIB_RGB_COLORS);
    } /* if lpDib != NULL */
} /* PaintDibInWindow() */
