/******************************************************************************/
/* TextMem.h -- Huge Text Area support.  Allows the allocation of one huge    */
/* Global Memory area, then allocating pieces of it, returning far pointers.  */
/* Contains routines to Create, Destroy, Reset, and Allocate text blocks.     */
/******************************************************************************/

typedef struct {
    HGLOBAL textGlobalArea;
    char huge *textGlobalMemBase;
    char huge *textGlobalMemPoint;
    long size;
    long bytesFree;
} TextRegionStruct;

TextRegionStruct far * TextCreate(unsigned long size);
void TextDestroy(TextRegionStruct far *textRegion);
void TextReset(TextRegionStruct far *textRegion);
char huge *TextMalloc(int allocSize, TextRegionStruct far *textRegion);
