#include <gtest/gtest.h>
#include "tcamviewer/terminal.hpp"

using namespace tcamviewer;

TEST(TerminalTest, EscapeSequences) {
    EXPECT_EQ(Terminal::setFgRgb(255, 128, 0), "\x1b[38;2;255;128;0m");
    EXPECT_EQ(Terminal::setBgRgb(10, 20, 30), "\x1b[48;2;10;20;30m");
    EXPECT_EQ(Terminal::cursorHome(), "\x1b[H");
    EXPECT_EQ(Terminal::cursorMoveTo(15, 7), "\x1b[7;15H");
    EXPECT_EQ(Terminal::clearScreen(), "\x1b[2J\x1b[H");
    EXPECT_EQ(Terminal::resetStyle(), "\x1b[0m");
    EXPECT_STREQ(Terminal::HALF_BLOCK, "\xE2\x96\x80");
}

TEST(TerminalTest, TerminalSize) {
    TerminalSize size = Terminal::getSize();
    EXPECT_GT(size.cols, 0);
    EXPECT_GT(size.rows, 0);
}
