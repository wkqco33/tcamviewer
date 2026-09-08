#include "tcamviewer/terminal.hpp"
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

namespace tcamviewer {

TerminalSize Terminal::getSize() {
    TerminalSize size{80, 24};
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_col > 0 && ws.ws_row > 0) {
            size.cols = ws.ws_col;
            size.rows = ws.ws_row;
            return size;
        }
    }

    const char* cols_env = std::getenv("COLUMNS");
    const char* rows_env = std::getenv("LINES");
    if (cols_env) {
        int c = std::atoi(cols_env);
        if (c > 0) size.cols = c;
    }
    if (rows_env) {
        int r = std::atoi(rows_env);
        if (r > 0) size.rows = r;
    }

    return size;
}

void Terminal::enterAlternateScreen() {
    std::cout << "\x1b[?1049h" << std::flush;
}

void Terminal::leaveAlternateScreen() {
    std::cout << "\x1b[?1049l" << std::flush;
}

void Terminal::hideCursor() {
    std::cout << "\x1b[?25l" << std::flush;
}

void Terminal::showCursor() {
    std::cout << "\x1b[?25h" << std::flush;
}

std::string Terminal::cursorHome() {
    return "\x1b[H";
}

std::string Terminal::cursorMoveTo(int col, int row) {
    return "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

std::string Terminal::clearScreen() {
    return "\x1b[2J\x1b[H";
}

std::string Terminal::resetStyle() {
    return "\x1b[0m";
}

std::string Terminal::setFgRgb(uint8_t r, uint8_t g, uint8_t b) {
    return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string Terminal::setBgRgb(uint8_t r, uint8_t g, uint8_t b) {
    return "\x1b[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

} // namespace tcamviewer
