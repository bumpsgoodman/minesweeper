//***************************************************************************
// 
// 파일: console_util.h
// 
// 설명: 콘솔 유틸리티
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/10/28
// 
//***************************************************************************

#ifndef CONSOLE_UTIL_H
#define CONSOLE_UTIL_H

#include "safe99_common/defines.h"

void gotoxy(const int x, const int y);
void visible_cursor(const bool b_visible);

#endif // CONSOLE_UTIL_H