//***************************************************************************
// 
// 파일: console_util.c
// 
// 설명: 콘솔 유틸리티
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/10/28
// 
//***************************************************************************

#include "precompiled.h"

void gotoxy(const int x, const int y)
{
    COORD pos = {x,y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void visible_cursor(const bool b_visible)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursor_info;

    GetConsoleCursorInfo(out, &cursor_info);
    cursor_info.bVisible = b_visible; // set the cursor visibility
    SetConsoleCursorInfo(out, &cursor_info);
}