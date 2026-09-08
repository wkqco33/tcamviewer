#include <gtest/gtest.h>
#include "tcamviewer/renderer.hpp"
#include <vector>
#include <chrono>

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

TEST(RendererTest, Rotation90) {
    RenderConfig config;
    config.targetCols = 1;
    config.targetRows = 1;
    config.useDiff = false;
    config.rotation = 90;

    Renderer renderer(config);
    EXPECT_EQ(renderer.getRotation(), 90);

    // 2x1 image: (0,0)=Red (255,0,0), (1,0)=Blue (0,0,255)
    std::vector<uint8_t> rgb = {
        255, 0, 0,    0, 0, 255
    };

    std::string ansi = renderer.generateAnsiString(rgb.data(), 2, 1, 6, false);
    EXPECT_FALSE(ansi.empty());
    // Rotated 90 deg clockwise: (0,0) becomes top, (1,0) becomes bottom
    EXPECT_NE(ansi.find("38;2;255;0;0"), std::string::npos);
    EXPECT_NE(ansi.find("48;2;0;0;255"), std::string::npos);
}

TEST(RendererTest, DynamicRotationChange) {
    RenderConfig config;
    config.targetCols = 1;
    config.targetRows = 1;
    config.useDiff = false;
    config.rotation = 0;

    Renderer renderer(config);
    EXPECT_EQ(renderer.getRotation(), 0);

    renderer.setRotation(270);
    EXPECT_EQ(renderer.getRotation(), 270);

    renderer.setRotation(450); // 450 % 360 = 90
    EXPECT_EQ(renderer.getRotation(), 90);
}

TEST(RendererTest, AspectRatioLetterbox) {
    RenderConfig config;
    config.targetCols = 4;
    config.targetRows = 4; // 4 cols x 4 rows
    config.useDiff = false;
    config.keepAspectRatio = true;

    Renderer renderer(config);
    EXPECT_TRUE(renderer.isKeepAspectRatio());

    // 4x2 bright red image
    std::vector<uint8_t> rgb(4 * 2 * 3, 255);

    std::string ansi = renderer.generateAnsiString(rgb.data(), 4, 2, 12, false);
    EXPECT_FALSE(ansi.empty());
    // Should render red in content row
    EXPECT_NE(ansi.find("38;2;255;255;255"), std::string::npos);
    // Should also render black padding (0;0;0) in letterbox rows
    EXPECT_NE(ansi.find("38;2;0;0;0"), std::string::npos);
}

TEST(RendererTest, AspectRatioPillarbox) {
    RenderConfig config;
    config.targetCols = 8;
    config.targetRows = 2; // Wide terminal (8x4 pixels)
    config.useDiff = false;
    config.keepAspectRatio = true;

    Renderer renderer(config);

    // 2x4 bright green image (Tall image)
    std::vector<uint8_t> rgb(2 * 4 * 3, 0);
    for (size_t i = 1; i < rgb.size(); i += 3) {
        rgb[i] = 255; // Green
    }

    std::string ansi = renderer.generateAnsiString(rgb.data(), 2, 4, 6, false);
    EXPECT_FALSE(ansi.empty());
    // Should have green content
    EXPECT_NE(ansi.find("38;2;0;255;0"), std::string::npos);
    // Should have black pillarbox padding
    EXPECT_NE(ansi.find("38;2;0;0;0"), std::string::npos);
}

TEST(RendererTest, ToggleKeepAspectRatio) {
    RenderConfig config;
    config.targetCols = 4;
    config.targetRows = 4;
    config.keepAspectRatio = true;

    Renderer renderer(config);
    EXPECT_TRUE(renderer.isKeepAspectRatio());

    renderer.setKeepAspectRatio(false);
    EXPECT_FALSE(renderer.isKeepAspectRatio());
}

TEST(RendererTest, BenchmarkPerformance) {
    RenderConfig config;
    config.targetCols = 160;
    config.targetRows = 50; // 160x100 resolution (typical full screen terminal)
    config.useDiff = true;

    Renderer renderer(config);

    // Simulate 100 frames of dynamic content (HD 1280x720 downscaled)
    std::vector<uint8_t> frame(640 * 360 * 3, 0);
    auto start = std::chrono::steady_clock::now();

    for (int f = 0; f < 100; ++f) {
        // Slight perturbation per frame
        frame[(f * 17) % frame.size()] = static_cast<uint8_t>(f % 256);
        std::string ansi = renderer.generateAnsiString(frame.data(), 640, 360, 640 * 3, false);
        EXPECT_FALSE(ansi.empty());
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 100 frames on 160x50 terminal should comfortably complete within 1000ms (>100 FPS)
    EXPECT_LT(elapsedMs, 1000);
}

