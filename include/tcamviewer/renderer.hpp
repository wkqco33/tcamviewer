#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "terminal.hpp"

namespace tcamviewer {

struct RenderConfig {
    int targetCols{0};   // 0 = auto-detect terminal width
    int targetRows{0};   // 0 = auto-detect terminal height
    bool useDiff{true};  // Frame dirty-diff optimization
    bool altScreen{false};
    bool hideCursor{true};
    int rotation{0};     // Rotation in degrees: 0, 90, 180, 270 (Clockwise)
};

struct CellColor {
    uint8_t topR{0}, topG{0}, topB{0};
    uint8_t botR{0}, botG{0}, botB{0};

    bool operator==(const CellColor& o) const {
        return topR == o.topR && topG == o.topG && topB == o.topB &&
               botR == o.botR && botG == o.botG && botB == o.botB;
    }
    bool operator!=(const CellColor& o) const {
        return !(*this == o);
    }
};

class Renderer {
public:
    explicit Renderer(const RenderConfig& config = RenderConfig{});
    ~Renderer();

    // Update target terminal dimensions
    void resize(int cols, int rows);

    // Render an RGB24 frame (width x height, 3 bytes per pixel) directly to stdout
    void renderRgb24(const uint8_t* rgb, int width, int height, int stride = 0);

    // Render BGR24 frame (e.g. from OpenCV or ROS2 bgr8) directly to stdout
    void renderBgr24(const uint8_t* bgr, int width, int height, int stride = 0);

    // Generates the ANSI string without printing to stdout (for testing or custom transport)
    std::string generateAnsiString(const uint8_t* data, int width, int height, int stride = 0, bool isBgr = false);

    // Clear internal diff buffer so next frame is rendered completely fresh
    void invalidateCache();

    // Accessors
    int getCols() const { return cols_; }
    int getRows() const { return rows_; }
    int getRotation() const { return config_.rotation; }
    void setRotation(int degrees);
    bool isDiffEnabled() const { return config_.useDiff; }
    void setDiffEnabled(bool enable) { config_.useDiff = enable; }

private:
    void updateDimensions();
    void buildCellGrid(const uint8_t* src, int srcW, int srcH, int stride, bool isBgr,
                       std::vector<CellColor>& outGrid);

    RenderConfig config_;
    int cols_{80};
    int rows_{24};
    std::vector<CellColor> prevGrid_;
    std::string outputBuffer_;
    bool isFirstFrame_{true};
};

} // namespace tcamviewer
