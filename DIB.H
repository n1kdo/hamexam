DWORD GetDibInfoHeaderSize (BYTE huge *);
WORD GetDibWidth (BYTE huge *);
WORD GetDibHeight (BYTE huge *);
BYTE huge * GetDibBitsAddr (BYTE huge *);
BYTE huge * ReadDib (LPSTR);
void FreeDib(BYTE huge **);
void PaintDibInWindow(BYTE huge *, HDC);

