/******************************************************************************/
/* TextMem.C -- Huge Text Area support.  Allows the allocation of one huge    */
/* Global Memory area, then allocating pieces of it, returning far pointers.  */
/* Contains routines to Create, Destroy, Reset, and Allocate text blocks.     */
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

#include <dos.h>
#include "TextMem.h"

extern HWND hwndMain; /* used for error message boxes */

/******************************************************************************/
/* TextCreate -- create a huge text area.                                     */
/******************************************************************************/
TextRegionStruct far * TextCreate(unsigned long size)
{
    TextRegionStruct far *textRegion;

    textRegion = (TextRegionStruct far *) GlobalAllocPtr(GPTR, sizeof(TextRegionStruct));
    textRegion->size = size;
    textRegion->bytesFree = textRegion->size;

    textRegion->textGlobalArea = GlobalAlloc(GPTR, textRegion->size);
    if (textRegion->textGlobalArea == NULL)
    {
        MessageBox(hwndMain, "Could not allocate enough memory!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        GlobalFreePtr(textRegion);
        textRegion->bytesFree = 0;
        return(NULL);
    }

    textRegion->textGlobalMemBase = GlobalLock(textRegion->textGlobalArea);
    if (textRegion->textGlobalMemBase == NULL)
    {
        MessageBox(hwndMain, "Could not lock enough memory!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        GlobalFreePtr(textRegion);
        GlobalFree(textRegion->textGlobalArea);
        return(NULL);
    }
    textRegion->textGlobalMemPoint = textRegion->textGlobalMemBase;
    return (textRegion);
} /* TextCreate() */

/******************************************************************************/
/* TextDestroy -- free a huge text area.                                      */
/******************************************************************************/
void TextDestroy(TextRegionStruct far *textRegion)
{
    GlobalUnlock(textRegion->textGlobalArea);
    GlobalFree(textRegion->textGlobalArea);
    GlobalFreePtr(textRegion);
} /* TextDestroy() */

/******************************************************************************/
/* TextReset -- reset (empty) a huge text area.                               */
/******************************************************************************/
void TextReset(TextRegionStruct far *textRegion)
{
    textRegion->bytesFree = textRegion->size;
    textRegion->textGlobalMemPoint = textRegion->textGlobalMemBase;
} /* TextReset() */

/******************************************************************************/
/* TextMalloc -- get space from text buffer and return a pointer to it.       */
/* If no space is left, return NULL.                                          */
/******************************************************************************/
char huge *TextMalloc(int allocSize, TextRegionStruct far *textRegion)
{
    char huge *pos;
    long result;
#ifndef __WIN32__
    unsigned long wastedChunk;
    unsigned long offset;
#endif
    result = textRegion->bytesFree - allocSize;
    if (result < 0)
    {
        return(NULL);
    }

/******************************** HACK ALERT! ********************************/
/* below lies the bones of some code that prevents a region of memory from   */
/* being allocated across the boundary between segments.  This is needed     */
/* because the library/API functions don't know how to deal with huge        */
/* pointers, and it seems that Borland C++ 4.5 does not normalize the huge   */
/* pointers when they are modified, only when they wrap across segment       */
/* boundaries.                                                               */
/******************************** HACK ALERT! ********************************/

#ifndef __WIN32__
    offset = FP_OFF(textRegion->textGlobalMemPoint);
    if (offset + allocSize > 0xffff)
    {
        wastedChunk = 0x10000L - offset;
        textRegion->bytesFree -= wastedChunk;
        textRegion->textGlobalMemPoint += wastedChunk;
    }
#endif
    pos = textRegion->textGlobalMemPoint;
    textRegion->bytesFree -= allocSize;
    textRegion->textGlobalMemPoint += allocSize;
    return(pos);
} /* TextMalloc */

