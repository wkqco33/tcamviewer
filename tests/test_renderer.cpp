#include <gtest/gtest.h>
#include "tcamviewer/renderer.hpp"
#include <vector>

using namespace tcamviewer;

TEST(RendererTest, BasicRgbRendering) {
    RenderConfig config;
    config.targetCols = 2;
    config.targetRows = 1; // 2 cols x 1 row = 2x2 image
    config.useDiff = false;
    config.altScreen = false;
    config.hideCursor = false;

    Renderer renderer(config);
    EXPECT_EQ(renderer.getCols(), 2);
    EXPECT_EQ(renderer.getRows(), 1);

    // 2x2 RGB image:
    // (0,0): Red (255,0,0),    (1,0): Green (0,255,0)
    // (0,1): Blue (0,0,255),   (1,1): White (255,255,255)
    std::vector<uint8_t> rgb = {
        255, 0, 0,      0, 255, 0,
        0, 0, 255,      255, 255, 255
    };

    std::string ansi = renderer.generateAnsiString(rgb.data(), 2, 2, 6, false);
    EXPECT_FALSE(ansi.empty());
    // Should contain Half-block character
    EXPECT_NE(ansi.find(Terminal::HALF_BLOCK), std::string::npos);
    // Should contain Red FG
    EXPECT_NE(ansi.find("38;2;255;0;0"), std::string::npos);
    // Should contain Blue BG
    EXPECT_NE(ansi.find("48;2;0;0;255"), std::string::npos);
}

TEST(RendererTest, BgrConversion) {
    RenderConfig config;
    config.targetCols = 1;
    config.targetRows = 1; // 1x2 image
    config.useDiff = false;

    Renderer renderer(config);

    // 1x2 BGR image:
    // top: Blue in BGR is (255, 0, 0) -> in RGB is (0, 0, 255)
    // bot: Red in BGR is (0, 0, 255) -> in RGB is (255, 0, 0)
    std::vector<uint8_t> bgr = {
        255, 0, 0,
        0, 0, 255
    };

    std::string ansi = renderer.generateAnsiString(bgr.data(), 1, 2, 3, true);
    // Top pixel RGB is (0, 0, 255) -> FG should be 38;2;0;0;255
    EXPECT_NE(ansi.find("38;2;0;0;255"), std::string::npos);
    // Bottom pixel RGB is (255, 0, 0) -> BG should be 48;2;255;0;0
    EXPECT_NE(ansi.find("48;2;255;0;0"), std::string::npos);
}

TEST(RendererTest, DirtyDiffOptimization) {
    RenderConfig config;
    config.targetCols = 2;
    config.targetRows = 2;
    config.useDiff = true;

    Renderer renderer(config);

    std::vector<uint8_t> frame(2 * 4 * 3, 100); // Gray 2x4 image

    // First frame render
    std::string ansi1 = renderer.generateAnsiString(frame.data(), 2, 4, 6);
    EXPECT_FALSE(ansi1.empty());

    // Second frame render with exact same data
    std::string ansi2 = renderer.generateAnsiString(frame.data(), 2, 4, 6);
    // Because no pixels changed, ansi2 should be minimal (just home/reset) and contain NO half blocks
    EXPECT_EQ(ansi2.find(Terminal::HALF_BLOCK), std::string::npos);

    // Invalidate cache
    renderer.invalidateCache();
    std::string ansi3 = renderer.generateAnsiString(frame.data(), 2, 4, 6);
    // Should re-render everything
    EXPECT_NE(ansi3.find(Terminal::HALF_BLOCK), std::string::npos);
}

TEST(RendererTest, ResizeHandling) {
    RenderConfig config;
    config.targetCols = 10;
    config.targetRows = 5;

    Renderer renderer(config);
    EXPECT_EQ(renderer.getCols(), 10);
    EXPECT_EQ(renderer.getRows(), 5);

    renderer.resize(20, 10);
    EXPECT_EQ(renderer.getCols(), 20);
    EXPECT_EQ(renderer.getRows(), 10);
}
