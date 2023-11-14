//***************************************************************************
// 
// 파일: key_event.h
// 
// 설명: 키보드/마우스 이벤트
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/10/18
// 
//***************************************************************************

#include "safe99_common/defines.h"

typedef enum keyboard_state
{
    KEYBOARD_STATE_UP = 0x00,
    KEYBOARD_STATE_DOWN = 0x01,
    KEYBOARD_STATE_PRESSED = 0x10
} keyboard_state_t;

typedef enum mouse_state
{
    MOUSE_STATE_UP,
    MOUSE_STATE_DOWN
} mouse_state_t;

// 키보드 이벤트

void on_key_down(const uint8_t key);
void on_key_up(const uint8_t key);
void reset_keyboard(void);
keyboard_state_t get_key_state(const uint8_t key);

// 마우스 이벤트

void on_move_mouse(const int x, const int y);
void on_down_left_mouse(void);
void on_up_left_mouse(void);
void on_down_right_mouse(void);
void on_up_right_mouse(void);

void release_mouse(void);

int get_mouse_x(void);
int get_mouse_y(void);

mouse_state_t get_left_mouse_state(void);
mouse_state_t get_right_mouse_state(void);