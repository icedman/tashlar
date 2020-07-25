#ifndef DOTS_H
#define DOTS_H

#include <cstring>

int buildUpDots(int c, int row, int left, int right);
const wchar_t* wcharFromDots(int c);

int* buildUpDotsForLines(char* row1, char* row2, char* row3, char* row4, float textCompress = 4, int bufferWidth = 25);

#endif // DOTS_H