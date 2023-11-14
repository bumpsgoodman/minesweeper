//***************************************************************************
// 
// 파일: main.c
// 
// 설명: main 함수
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/10/27
// 
//***************************************************************************

#include "precompiled.h"
#include "game.h"

HINSTANCE g_hinstance;
HWND g_hwnd;

HRESULT init_window(const int width, const int height);
LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int main(void)
{
    int rows;
    int cols;
    int num_mines;

    // 모니터 해상도 구하기
    HMONITOR monitor = MonitorFromWindow(GetConsoleWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    const int monitor_width = info.rcMonitor.right - info.rcMonitor.left;
    const int monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;

    const int num_max_rows = (monitor_height - INFO_HEIGHT * 3) / SPRITE_TILE_HEIGHT;
    const int num_max_cols = monitor_width / SPRITE_TILE_WIDTH;

    printf("rows(9 ~ %d)\n> ", num_max_rows);
    scanf("%d", &rows);
    printf("\n");

    if (rows < 9 || rows > num_max_rows)
    {
        MessageBox(NULL, L"Out of rows", L"rows", MB_OK | MB_ICONERROR);
        return 0;
    }

    printf("cols(9 ~ %d)\n> ", num_max_cols);
    scanf("%d", &cols);
    printf("\n");

    if (cols < 9 || cols > num_max_cols)
    {
        MessageBox(NULL, L"Out of cols", L"cols", MB_OK | MB_ICONERROR);
        return 0;
    }

    printf("num of mines(1 ~ %d)\n> ", rows * cols);
    scanf("%d", &num_mines);
    printf("\n");

    if (num_mines < 1 || num_mines >= rows * cols)
    {
        MessageBox(NULL, L"Out of mines", L"num of mines", MB_OK | MB_ICONERROR);
        return 0;
    }

    const int window_width = cols * SPRITE_TILE_WIDTH;
    const int window_height = rows * SPRITE_TILE_HEIGHT + INFO_HEIGHT;

    if (FAILED(init_window(window_width, window_height)))
    {
        ASSERT(false, "Failed to init window");
        return 0;
    }

    gp_game = (game_t*)malloc(sizeof(game_t));
    if (gp_game == NULL)
    {
        ASSERT(false, "Failed to malloc app");
        return 0;
    }

    if (!init_game(rows, cols, num_mines))
    {
        ASSERT(false, "Failed to init app");
        return 0;
    }

    // Main message loop
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            tick_game();
        }
    }

    shutdown_game();
    SAFE_FREE(gp_game);

    return (int)msg.wParam;
}

HRESULT init_window(const int width, const int height)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = wnd_proc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandleW(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"safe99";
    wcex.hIconSm = NULL;
    if (!RegisterClassEx(&wcex))
    {
        return E_FAIL;
    }

    // Create window
    g_hinstance = wcex.hInstance;
    RECT rc = { 0, 0, width, height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindow(wcex.lpszClassName, L"safe99 - minesweeper",
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                          CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, wcex.hInstance,
                          NULL);
    if (!g_hwnd)
    {
        return E_FAIL;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);

    return S_OK;
}

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
    {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        on_down_left_mouse();
        break;
    }
    case WM_LBUTTONUP:
    {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        on_up_left_mouse();
        break;
    }
    case WM_RBUTTONDOWN:
    {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        on_down_right_mouse();
        break;
    }
    case WM_RBUTTONUP:
    {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        on_up_right_mouse();
        break;
    }
    case WM_MOUSEMOVE:
    {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        on_move_mouse(x, y);
        break;
    }

    case WM_KEYDOWN:
        on_key_down((uint8_t)wParam);
        break;
    case WM_KEYUP:
        on_key_up((uint8_t)wParam);
        break;

    case WM_SIZE:
        if (gp_game != NULL)
        {
            update_window_size_game();
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}