#include "pch.h"

void set_up_console()
{
    auto const console_window = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD startup_console_mode;
    GetConsoleMode(console_window, &startup_console_mode);
    SetConsoleMode(console_window, startup_console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void on_driver_not_found()
{
}
