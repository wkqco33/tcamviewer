#pragma once

#include <cstdint>
#include <string>

namespace tcamviewer {

struct TerminalSize {
    int cols{80};
    int rows{24};
};

class Terminal {
public:
    // Get current terminal window size in columns and rows
    static TerminalSize getSize();

    // Enable/disable alternate screen buffer
    static void enterAlternateScreen();
    static void leaveAlternateScreen();

    // Show/hide cursor
    static void hideCursor();
    static void showCursor();

    // Cursor position helpers
    static std::string cursorHome();
    static std::string cursorMoveTo(int col, int row);

    // Clear entire screen
    static std::string clearScreen();

    // Reset style & colors
    static std::string resetStyle();

    // 24-bit TrueColor ANSI escape sequences
    static std::string setFgRgb(uint8_t r, uint8_t g, uint8_t b);
    static std::string setBgRgb(uint8_t r, uint8_t g, uint8_t b);

    // Half-block character (U+2580: "▀")
    static constexpr const char* HALF_BLOCK = "\xE2\x96\x80";
};

} // namespace tcamviewer
