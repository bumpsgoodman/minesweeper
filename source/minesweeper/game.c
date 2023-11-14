//***************************************************************************
// 
// 파일: game.c
// 
// 설명: 게임
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/09/12
// 
//***************************************************************************

#include "precompiled.h"
#include "game.h"

// 전역 변수
extern HWND g_hwnd;

// 전역 함수
static void update(const float delta_time);
static void draw(void);

static void print_info(void);

static void make_mine(bool* p_mines, const int rows, const int cols, const int num_mines);

static bool is_valid_position(const int x, const int y);
static void open_tile(const int x, const int y);

bool init_game(const int rows, const int cols, const int num_mines)
{
    ASSERT(rows >= 9, "width < 9");
    ASSERT(cols >= 9, "height < 9");
    ASSERT(num_mines > 0, "num_mines == 0");

    visible_cursor(false);
    srand((unsigned int)time(NULL));

    memset(gp_game, 0, sizeof(game_t));

    // DLL 로드
    {
    #if defined(_WIN64)
        gp_game->h_renderer_dll = LoadLibrary(L"safe99_soft_renderer_x64.dll");
        gp_game->h_file_system_dll = LoadLibrary(L"safe99_file_system_x64.dll");
    #else
        gp_game->h_renderer_dll = LoadLibrary(L"safe99_soft_renderer_x86.dll");
        gp_game->h_file_system_dll = LoadLibrary(L"safe99_file_system_x86.dll");
    #endif // _WIN64

        if (gp_game->h_renderer_dll == NULL
            || gp_game->h_file_system_dll == NULL)
        {
            ASSERT(false, "Failed load DLL");
            goto failed_init;
        }

        // 인스턴스 생성 함수
        create_instance_func pf_create_renderer = (create_instance_func)GetProcAddress(gp_game->h_renderer_dll, "create_instance");
        create_instance_func pf_create_file_system = (create_instance_func)GetProcAddress(gp_game->h_file_system_dll, "create_instance");

        // 렌더러 초기화
        pf_create_renderer(&gp_game->p_renderer);
        if (!gp_game->p_renderer->vtbl->initialize(gp_game->p_renderer, g_hwnd, false))
        {
            ASSERT(false, "Failed init renderer");
            goto failed_init;
        }

        // 파일 시스템 초기화
        pf_create_file_system(&gp_game->p_file_system);
        if (!gp_game->p_file_system->vtbl->initialize(gp_game->p_file_system))
        {
            ASSERT(false, "Failed init file system");
            goto failed_init;
        }
    }

    // 프레임 초기화
    {
        gp_game->update_ticks_per_frame = 1.0f / 60.0f;
        gp_game->fps = 0;

        timer_init(&gp_game->frame_timer);
    }

    // 텍스쳐 로드
    {
        // 타일 로드
        if (!gp_game->p_file_system->vtbl->load_a8r8g8b8_dds(gp_game->p_file_system,
                                                             "..\\..\\sprite\\tiles.dds",
                                                             &gp_game->p_tiles_texture))
        {
            MessageBox(NULL, L"Failed to load texture", L"Error", MB_OK | MB_ICONERROR);
            ASSERT(false, "Failed to load texture");
            goto failed_init;
        }

        // 숫자 로드
        if (!gp_game->p_file_system->vtbl->load_a8r8g8b8_dds(gp_game->p_file_system,
                                                             "..\\..\\sprite\\numbers.dds",
                                                             &gp_game->p_numbers_texture))
        {
            MessageBox(NULL, L"Failed to load texture", L"Error", MB_OK | MB_ICONERROR);
            ASSERT(false, "Failed to load texture");
            goto failed_init;
        }

        // 얼굴 로드
        if (!gp_game->p_file_system->vtbl->load_a8r8g8b8_dds(gp_game->p_file_system,
                                                             "..\\..\\sprite\\faces.dds",
                                                             &gp_game->p_faces_texture))
        {
            MessageBox(NULL, L"Failed to load texture", L"Error", MB_OK | MB_ICONERROR);
            ASSERT(false, "Failed to load texture");
            goto failed_init;
        }
    }

    // 게임 초기화
    {
        gp_game->b_running = true;
        gp_game->rows = rows;
        gp_game->cols = cols;
        gp_game->num_mines = num_mines;
        gp_game->num_max_mines = num_mines;
        gp_game->b_left_mouse_pressed = false;
        gp_game->b_right_mouse_pressed = false;
        gp_game->num_tiles = rows * cols;

        // 지뢰 초기화
        gp_game->pa_mines = (bool*)malloc(sizeof(bool) * rows * cols);
        if (gp_game->pa_mines == NULL)
        {
            ASSERT(false, "Failed to malloc mines");
            goto failed_init;
        }
        memset(gp_game->pa_mines, false, sizeof(bool) * rows * cols);

        // 타일 초기화
        gp_game->pa_tiles = (tile_t*)malloc(sizeof(tile_t) * rows * cols);
        if (gp_game->pa_tiles == NULL)
        {
            ASSERT(false, "Failed to malloc tiles");
            goto failed_init;
        }
        memset(gp_game->pa_tiles, TILE_BLIND, sizeof(tile_t) * rows * cols);

        // 얼굴 위치 초기화
        const int WINDOW_WIDTH = (int)gp_game->p_renderer->vtbl->get_width(gp_game->p_renderer);
        const int WINDOW_HEIGHT = (int)gp_game->p_renderer->vtbl->get_height(gp_game->p_renderer);

        gp_game->face_x = WINDOW_WIDTH / 2 - SPRITE_FACE_WIDTH / 2;
        gp_game->face_y = INFO_HEIGHT / 2 - SPRITE_FACE_HEIGHT / 2;

        // 게임 타이머 초기화
        gp_game->game_time = 0;
        timer_init(&gp_game->game_timer);

        // 지뢰 생성
        make_mine(gp_game->pa_mines, rows, cols, num_mines);
    }

    return true;

failed_init:
    shutdown_game();
    return false;
}

void shutdown_game(void)
{
    // 게임 리소스 해제
    SAFE_FREE(gp_game->pa_tiles);
    SAFE_FREE(gp_game->pa_mines);

    // 텍스쳐 해제
    SAFE_RELEASE(gp_game->p_tiles_texture);
    SAFE_RELEASE(gp_game->p_numbers_texture);
    SAFE_RELEASE(gp_game->p_faces_texture);

    // DLL 객체 해제
    SAFE_RELEASE(gp_game->p_renderer);
    SAFE_RELEASE(gp_game->p_file_system);
    
    // DLL 핸들 해제
    FreeLibrary(gp_game->h_renderer_dll);
    FreeLibrary(gp_game->h_file_system_dll);

    memset(gp_game, 0, sizeof(game_t));
}

void tick_game(void)
{
    // 업데이트 시작
    {
        static float start_time;

        const float end_time = timer_get_time(&gp_game->frame_timer);
        const float elapsed_time = end_time - start_time;
        if (elapsed_time >= gp_game->update_ticks_per_frame)
        {
            update(elapsed_time);

            start_time = end_time;
        }
    }

    // 렌더링 시작
    {
        static float start_time;
        static int frame_count;

        draw();
        print_info();

        const float end_time = timer_get_time(&gp_game->frame_timer);
        const float elapsed_time = (end_time - start_time) * 1000.0f;
        if (elapsed_time >= 1000.0f)
        {
            gp_game->fps = frame_count;
            frame_count = 0;

            start_time = end_time;
        }

        ++frame_count;
    }
}

static void update(const float delta_time)
{
    const int WINDOW_WIDTH = (int)gp_game->p_renderer->vtbl->get_width(gp_game->p_renderer);
    const int WINDOW_HEIGHT = (int)gp_game->p_renderer->vtbl->get_height(gp_game->p_renderer);

    const int ROWS = gp_game->rows;
    const int COLS = gp_game->cols;

    if (!gp_game->b_left_mouse_pressed && get_left_mouse_state() == MOUSE_STATE_DOWN)
    {
        gp_game->b_left_mouse_pressed = true;
    }

    if (gp_game->b_right_mouse_pressed && get_right_mouse_state() == MOUSE_STATE_UP)
    {
        gp_game->b_right_mouse_pressed = false;
    }

    if (gp_game->b_left_mouse_pressed && get_left_mouse_state() == MOUSE_STATE_UP)
    {
        const int mouse_x = get_mouse_x();
        const int mouse_y = get_mouse_y();

        // 얼굴 클릭 시 게임 재시작
        if (mouse_x >= gp_game->face_x && mouse_x <= gp_game->face_x + SPRITE_FACE_WIDTH
            && mouse_y >= gp_game->face_y && mouse_y <= gp_game->face_y + SPRITE_FACE_HEIGHT)
        {
            gp_game->num_mines = gp_game->num_max_mines;
            gp_game->num_tiles = gp_game->rows * gp_game->cols;
            gp_game->game_time = 0;
            gp_game->b_running = true;

            memset(gp_game->pa_mines, false, sizeof(bool) * gp_game->rows * gp_game->cols);
            memset(gp_game->pa_tiles, TILE_BLIND, sizeof(tile_t) * gp_game->rows * gp_game->cols);
            make_mine(gp_game->pa_mines, gp_game->rows, gp_game->cols, gp_game->num_max_mines);

            gp_game->b_left_mouse_pressed = false;
            gp_game->b_right_mouse_pressed = false;
        }

        if (!gp_game->b_running)
        {
            gp_game->b_left_mouse_pressed = false;
        }
    }

    if (!gp_game->b_running)
    {
        return;
    }

    static float start_time;
    const float end_time = timer_get_time(&gp_game->game_timer);
    const float elapsed = (end_time - start_time) * 1000.0f;
    if (elapsed >= 1000.0f)
    {
        ++gp_game->game_time;
        start_time = end_time;
    }

    if (gp_game->b_left_mouse_pressed && get_left_mouse_state() == MOUSE_STATE_UP)
    {
        const int mouse_x = (int)get_mouse_x();
        const int mouse_y = (int)get_mouse_y();

        // 스크린 좌표 -> 타일 좌표 변환
        const int tile_x = mouse_x / SPRITE_TILE_WIDTH;
        const int tile_y = (mouse_y - INFO_HEIGHT) / SPRITE_TILE_HEIGHT;

        // 타일 클릭 시
        if (mouse_x >= 0 && mouse_x < WINDOW_WIDTH
            && mouse_y >= INFO_HEIGHT && mouse_y < WINDOW_HEIGHT)
        {
            // 지뢰일 경우
            const bool b_gameover_mine = gp_game->pa_mines[tile_y * gp_game->cols + tile_x];
            if (b_gameover_mine)
            {
                // 지뢰가 있는 타일 열기
                for (int i = 0; i < gp_game->rows; ++i)
                {
                    for (int j = 0; j < gp_game->cols; ++j)
                    {
                        const bool b_mine = gp_game->pa_mines[i * gp_game->cols + j];
                        if (b_mine)
                        {
                            gp_game->pa_tiles[i * gp_game->cols + j] = TILE_MINE;
                        }
                    }
                }

                gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] = TILE_GAMEOVER_MINE;
                gp_game->b_running = false;
            }
            else if (gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] != TILE_FLAG)
            {
                open_tile(tile_x, tile_y);

                // 남은 타일의 수와 지뢰 개수가 같으면 승리
                if (gp_game->num_tiles == gp_game->num_max_mines)
                {
                    gp_game->num_mines = 0;
                    gp_game->b_running = false;
                }
            }
        }

        gp_game->b_left_mouse_pressed = false;
    }

    if (!gp_game->b_right_mouse_pressed && get_right_mouse_state() == MOUSE_STATE_DOWN)
    {
        const int mouse_x = (int)get_mouse_x();
        const int mouse_y = (int)get_mouse_y();

        if (mouse_x >= 0 && mouse_x < WINDOW_WIDTH
            && mouse_y >= INFO_HEIGHT && mouse_y < WINDOW_HEIGHT)
        {
            // 스크린 좌표 -> 타일 좌표 변환
            const int tile_x = mouse_x / SPRITE_TILE_WIDTH;
            const int tile_y = (mouse_y - INFO_HEIGHT) / SPRITE_TILE_HEIGHT;

            const tile_t tile = gp_game->pa_tiles[tile_y * gp_game->cols + tile_x];
            switch (tile)
            {
            case TILE_BLIND:
                --gp_game->num_mines;
                gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] = TILE_FLAG;
                break;
            case TILE_FLAG:
                gp_game->num_mines++;
                gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] = TILE_UNKNOWN;
                break;
            case TILE_UNKNOWN:
                gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] = TILE_BLIND;
                break;
            default:
                break;
            }
        }

        gp_game->b_right_mouse_pressed = true;
    }
}

void draw(void)
{
    const int WINDOW_WIDTH = (int)gp_game->p_renderer->vtbl->get_width(gp_game->p_renderer);
    const int WINDOW_HEIGHT = (int)gp_game->p_renderer->vtbl->get_height(gp_game->p_renderer);

    const int NUM_MINES_DIGIT0_X = SPRITE_NUMBER_WIDTH * 2;
    const int NUM_MINES_DIGIT0_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int NUM_MINES_DIGIT1_X = SPRITE_NUMBER_WIDTH * 1;
    const int NUM_MINES_DIGIT1_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int NUM_MINES_DIGIT2_X = SPRITE_NUMBER_WIDTH * 0;
    const int NUM_MINES_DIGIT2_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;

    const int TIMER_DIGIT0_X = WINDOW_WIDTH - SPRITE_NUMBER_WIDTH * 1;
    const int TIMER_DIGIT0_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int TIMER_DIGIT1_X = WINDOW_WIDTH - SPRITE_NUMBER_WIDTH * 2;
    const int TIMER_DIGIT1_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int TIMER_DIGIT2_X = WINDOW_WIDTH - SPRITE_NUMBER_WIDTH * 3;
    const int TIMER_DIGIT2_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;

    const int START_TILE_X = 0;
    const int START_TILE_Y = INFO_HEIGHT;

    const int mouse_x = get_mouse_x();
    const int mouse_y = get_mouse_y();

    int face_index = 0;

    gp_game->p_renderer->vtbl->clear(gp_game->p_renderer, argb_to_color(0xffc6c6c6));

    // 지뢰 개수 그리기
    {
        const char* p_bitmap = gp_game->p_numbers_texture->vtbl->get_bitmap(gp_game->p_numbers_texture);
        const int bitmap_width = (int)gp_game->p_numbers_texture->vtbl->get_width(gp_game->p_numbers_texture);
        const int bitmap_height = (int)gp_game->p_numbers_texture->vtbl->get_height(gp_game->p_numbers_texture);

        const int digit0_index = (gp_game->num_mines <= 0) ? 0 : gp_game->num_mines % 10;
        const int digit1_index = (gp_game->num_mines <= 0) ? 0 : gp_game->num_mines / 10 % 10;
        const int digit2_index = (gp_game->num_mines <= 0) ? 0 : gp_game->num_mines / 100 % 10;

        gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, NUM_MINES_DIGIT0_X, NUM_MINES_DIGIT0_Y,
                                                digit0_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT,
                                                (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
        gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, NUM_MINES_DIGIT1_X, NUM_MINES_DIGIT1_Y,
                                                digit1_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT,
                                                (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
        gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, NUM_MINES_DIGIT2_X, NUM_MINES_DIGIT2_Y,
                                                digit2_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT,
                                                (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
    }

    // 타이머 그리기
    {
        const char* p_bitmap = gp_game->p_numbers_texture->vtbl->get_bitmap(gp_game->p_numbers_texture);
        const int bitmap_width = (int)gp_game->p_numbers_texture->vtbl->get_width(gp_game->p_numbers_texture);
        const int bitmap_height = (int)gp_game->p_numbers_texture->vtbl->get_height(gp_game->p_numbers_texture);

        const int digit0_index = gp_game->game_time % 10;
        const int digit1_index = gp_game->game_time / 10 % 10;
        const int digit2_index = gp_game->game_time / 100 % 10;

        gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, TIMER_DIGIT0_X, TIMER_DIGIT0_Y,
                                                digit0_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT,
                                                (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
        gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, TIMER_DIGIT1_X, TIMER_DIGIT1_Y,
                                                digit1_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT,
                                                (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
        gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, TIMER_DIGIT2_X, TIMER_DIGIT2_Y,
                                                digit2_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT,
                                                (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
    }

    // 타일 그리기
    // 윈도우 좌표 -> 타일 좌표 변환
    const int tile_x = mouse_x / SPRITE_TILE_WIDTH;
    const int tile_y = (mouse_y - INFO_HEIGHT) / SPRITE_TILE_HEIGHT;

    const char* p_bitmap = gp_game->p_tiles_texture->vtbl->get_bitmap(gp_game->p_tiles_texture);
    const int bitmap_width = (int)gp_game->p_tiles_texture->vtbl->get_width(gp_game->p_tiles_texture);
    const int bitmap_height = (int)gp_game->p_tiles_texture->vtbl->get_height(gp_game->p_tiles_texture);

    for (int y = 0; y < gp_game->rows; ++y)
    {
        for (int x = 0; x < gp_game->cols; ++x)
        {
            tile_t tile = gp_game->pa_tiles[y * gp_game->cols + x];
            if (get_left_mouse_state() == MOUSE_STATE_DOWN && tile_x == x && tile_y == y)
            {
                face_index = 2;

                if (gp_game->b_running && tile == TILE_BLIND)
                {
                    tile = TILE_OPEN;
                }
            }

            switch (tile)
            {
            case TILE_BLIND:
            case TILE_OPEN:
            case TILE_FLAG:
            case TILE_UNKNOWN:
            case TILE_OPEN_UNKNOWN:
            case TILE_MINE:
            case TILE_GAMEOVER_MINE:
            case TILE_FLAG_MINE:
                if (!gp_game->b_running && gp_game->num_tiles == gp_game->num_max_mines
                    && tile == TILE_BLIND)
                {
                    tile = TILE_FLAG;
                }

                gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, START_TILE_X + x * SPRITE_TILE_WIDTH, START_TILE_Y + y * SPRITE_TILE_HEIGHT,
                                                        tile * SPRITE_TILE_WIDTH, 0, SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT,
                                                        (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
                break;
            case TILE_1:
            case TILE_2:
            case TILE_3:
            case TILE_4:
            case TILE_5:
            case TILE_6:
            case TILE_7:
            case TILE_8:
                gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, START_TILE_X + x * SPRITE_TILE_WIDTH, START_TILE_Y + y * SPRITE_TILE_HEIGHT,
                                                        (tile - 8) * SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT, SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT,
                                                        (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
                break;
            default:
                ASSERT(false, "Invalid tile");
                break;
            }
        }
    }

    // 얼굴 그리기
    {
        const char* p_bitmap = gp_game->p_faces_texture->vtbl->get_bitmap(gp_game->p_faces_texture);
        const int bitmap_width = (int)gp_game->p_faces_texture->vtbl->get_width(gp_game->p_faces_texture);
        const int bitmap_height = (int)gp_game->p_faces_texture->vtbl->get_height(gp_game->p_faces_texture);

        if (get_left_mouse_state() == MOUSE_STATE_DOWN
            && mouse_x >= gp_game->face_x && mouse_x <= gp_game->face_x + SPRITE_FACE_WIDTH
            && mouse_y >= gp_game->face_y && mouse_y <= gp_game->face_y + SPRITE_FACE_HEIGHT)
        {
            face_index = 1;
        }

        if (!gp_game->b_running)
        {
            face_index = (gp_game->num_tiles == gp_game->num_max_mines) ? 3 : 4;
        }

        gp_game->p_renderer->vtbl->draw_bitmap(gp_game->p_renderer, gp_game->face_x, gp_game->face_y,
                                                face_index * SPRITE_FACE_WIDTH, 0, SPRITE_FACE_WIDTH, SPRITE_FACE_HEIGHT,
                                                (uint32_t*)p_bitmap, bitmap_width, bitmap_height);
    }

    gp_game->p_renderer->vtbl->on_draw(gp_game->p_renderer);
}

static void print_info(void)
{
    gotoxy(0, 0);
    for (int y = 0; y < gp_game->rows; ++y)
    {
        for (int x = 0; x < gp_game->cols; ++x)
        {
            printf("%c ", gp_game->pa_mines[y * gp_game->cols + x] == true ? 'o' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

static void make_mine(bool* p_mines, const int rows, const int cols, const int num_mines)
{
    ASSERT(p_mines != NULL, "p_mines == NULL");

    int count = 0;
    while (count != num_mines)
    {
        const int index = rand() % (rows * cols);
        
        if (p_mines[index])
        {
            continue;
        }

        p_mines[index] = true;

        ++count;
    }
}

static bool is_valid_position(const int x, const int y)
{
    return (x >= 0 && x < gp_game->cols
            && y >= 0 && y < gp_game->rows);
}

static void open_tile(const int x, const int y)
{
    // 재귀 깊이 근사값
    // (rows * cols - num_max_mines) * 8

    int stack_index = 0;
    int* stack_x = (int*)malloc(sizeof(int) * (gp_game->rows * gp_game->cols - gp_game->num_max_mines) * 8);
    int* stack_y = (int*)malloc(sizeof(int) * (gp_game->rows * gp_game->cols - gp_game->num_max_mines) * 8);
    ASSERT(stack_x != NULL, "Failed to malloc stack_x");
    ASSERT(stack_y != NULL, "Failed to malloc stack_y");

    stack_x[stack_index] = x;
    stack_y[stack_index] = y;
    ++stack_index;

    while (stack_index > 0)
    {
        --stack_index;
        const int tile_x = stack_x[stack_index];
        const int tile_y = stack_y[stack_index];

        if (!is_valid_position(tile_x, tile_y))
        {
            continue;
        }

        const tile_t tile = gp_game->pa_tiles[tile_y * gp_game->cols + tile_x];
        if (tile != TILE_BLIND && tile != TILE_FLAG && tile != TILE_UNKNOWN)
        {
            continue;
        }

        int count = 0;
        count += (is_valid_position(tile_x, tile_y - 1) && gp_game->pa_mines[(tile_y - 1) * gp_game->cols + tile_x]);
        count += (is_valid_position(tile_x, tile_y + 1) && gp_game->pa_mines[(tile_y + 1) * gp_game->cols + tile_x]);
        count += (is_valid_position(tile_x - 1, tile_y) && gp_game->pa_mines[tile_y * gp_game->cols + (tile_x - 1)]);
        count += (is_valid_position(tile_x + 1, tile_y) && gp_game->pa_mines[tile_y * gp_game->cols + (tile_x + 1)]);
        count += (is_valid_position(tile_x - 1, tile_y - 1) && gp_game->pa_mines[(tile_y - 1) * gp_game->cols + (tile_x - 1)]);
        count += (is_valid_position(tile_x + 1, tile_y - 1) && gp_game->pa_mines[(tile_y - 1) * gp_game->cols + (tile_x + 1)]);
        count += (is_valid_position(tile_x - 1, tile_y + 1) && gp_game->pa_mines[(tile_y + 1) * gp_game->cols + (tile_x - 1)]);
        count += (is_valid_position(tile_x + 1, tile_y + 1) && gp_game->pa_mines[(tile_y + 1) * gp_game->cols + (tile_x + 1)]);

        if (gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] == TILE_FLAG)
        {
            ++gp_game->num_mines;
        }

        if (count == 0)
        {
            gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] = TILE_OPEN;

            stack_x[stack_index] = tile_x;
            stack_y[stack_index] = tile_y - 1;
            ++stack_index;

            stack_x[stack_index] = tile_x;
            stack_y[stack_index] = tile_y + 1;
            ++stack_index;

            stack_x[stack_index] = tile_x - 1;
            stack_y[stack_index] = tile_y;
            ++stack_index;

            stack_x[stack_index] = tile_x + 1;
            stack_y[stack_index] = tile_y;
            ++stack_index;

            stack_x[stack_index] = tile_x - 1;
            stack_y[stack_index] = tile_y - 1;
            ++stack_index;

            stack_x[stack_index] = tile_x + 1;
            stack_y[stack_index] = tile_y - 1;
            ++stack_index;

            stack_x[stack_index] = tile_x - 1;
            stack_y[stack_index] = tile_y + 1;
            ++stack_index;

            stack_x[stack_index] = tile_x + 1;
            stack_y[stack_index] = tile_y + 1;
            ++stack_index;
        }
        else
        {
            gp_game->pa_tiles[tile_y * gp_game->cols + tile_x] = TILE_1 + count - 1;
        }

        --gp_game->num_tiles;
    }

    free(stack_x);
    free(stack_y);
}