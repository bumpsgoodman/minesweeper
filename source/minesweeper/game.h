//***************************************************************************
// 
// 파일: game.h
// 
// 설명: 게임
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/09/12
// 
//***************************************************************************

#ifndef GAME_H
#define GAME_H

#include "safe99_common/defines.h"
#include "safe99_soft_renderer/i_soft_renderer.h"
#include "safe99_file_system/i_file_system.h"

#define SPRITE_TILE_WIDTH 16
#define SPRITE_TILE_HEIGHT 16
#define SPRITE_NUMBER_WIDTH 13
#define SPRITE_NUMBER_HEIGHT 23
#define SPRITE_FACE_WIDTH 24
#define SPRITE_FACE_HEIGHT 24

#define INFO_HEIGHT 48

typedef enum tile
{
    TILE_BLIND,
    TILE_OPEN,
    TILE_FLAG,
    TILE_UNKNOWN,
    TILE_OPEN_UNKNOWN,
    TILE_MINE,
    TILE_GAMEOVER_MINE,
    TILE_FLAG_MINE,
    TILE_1,
    TILE_2,
    TILE_3,
    TILE_4,
    TILE_5,
    TILE_6,
    TILE_7,
    TILE_8,
} tile_t;

typedef struct game
{
    // DLL 핸들
    HINSTANCE h_renderer_dll;
    HINSTANCE h_file_system_dll;

    // DLL 인스턴스
    i_soft_renderer_t* p_renderer;
    i_file_system_t* p_file_system;

    // 프레임
    timer_t frame_timer;
    float update_ticks_per_frame;
    int fps;

    // 지뢰찾기
    bool b_running;
    int rows;
    int cols;
    int num_mines;
    int num_max_mines;
    int num_tiles;
    bool* pa_mines;
    tile_t* pa_tiles;

    // 가운데 얼굴 위치
    int face_x;
    int face_y;

    bool b_left_mouse_pressed;
    bool b_right_mouse_pressed;

    // 게임 타이머
    timer_t game_timer;
    int game_time;

    // 텍스쳐
    i_texture_t* p_tiles_texture;
    i_texture_t* p_numbers_texture;
    i_texture_t* p_faces_texture;
} game_t;

game_t* gp_game;

START_EXTERN_C

bool init_game(const int rows, const int cols, const int num_mines);
void shutdown_game(void);
void tick_game(void);

FORCEINLINE bool is_running_game(void)
{
    return gp_game->b_running;
}

FORCEINLINE void update_window_size_game(void)
{
    gp_game->p_renderer->vtbl->update_window_size(gp_game->p_renderer);
}

END_EXTERN_C

#endif // GAME_H