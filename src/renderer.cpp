#include "tcamviewer/renderer.hpp"
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace tcamviewer {

Renderer::Renderer(const RenderConfig& config)
    : config_(config) {
    updateDimensions();
    if (config_.altScreen) {
        Terminal::enterAlternateScreen();
    }
    if (config_.hideCursor) {
        Terminal::hideCursor();
    }
}

Renderer::~Renderer() {
    if (config_.hideCursor) {
        Terminal::showCursor();
    }
    if (config_.altScreen) {
        Terminal::leaveAlternateScreen();
    }
    Terminal::resetStyle();
}

void Renderer::updateDimensions() {
    if (config_.targetCols > 0 && config_.targetRows > 0) {
        cols_ = config_.targetCols;
        rows_ = config_.targetRows;
    } else {
        auto sz = Terminal::getSize();
        cols_ = (config_.targetCols > 0) ? config_.targetCols : sz.cols;
        rows_ = (config_.targetRows > 0) ? config_.targetRows : sz.rows;
    }
    if (cols_ < 1) cols_ = 1;
    if (rows_ < 1) rows_ = 1;
}

void Renderer::resize(int cols, int rows) {
    config_.targetCols = cols;
    config_.targetRows = rows;
    updateDimensions();
    invalidateCache();
}

void Renderer::invalidateCache() {
    prevGrid_.clear();
    isFirstFrame_ = true;
}

void Renderer::setRotation(int degrees) {
    int rot = (degrees % 360 + 360) % 360;
    if (config_.rotation != rot) {
        config_.rotation = rot;
        invalidateCache();
    }
}

void Renderer::buildCellGrid(const uint8_t* src, int srcW, int srcH, int stride, bool isBgr,
                             std::vector<CellColor>& outGrid) {
    outGrid.resize(cols_ * rows_);
    if (!src || srcW <= 0 || srcH <= 0) return;

    if (stride <= 0) stride = srcW * 3;

    int rot = (config_.rotation % 360 + 360) % 360;
    int effW = (rot == 90 || rot == 270) ? srcH : srcW;
    int effH = (rot == 90 || rot == 270) ? srcW : srcH;

    int totalPixelH = rows_ * 2;

    auto mapCoord = [&](int eff_x, int eff_y, int& px, int& py) {
        if (rot == 90) {
            px = eff_y;
            py = srcH - 1 - eff_x;
        } else if (rot == 180) {
            px = srcW - 1 - eff_x;
            py = srcH - 1 - eff_y;
        } else if (rot == 270) {
            px = srcW - 1 - eff_y;
            py = eff_x;
        } else {
            px = eff_x;
            py = eff_y;
        }
        if (px < 0) px = 0;
        if (px >= srcW) px = srcW - 1;
        if (py < 0) py = 0;
        if (py >= srcH) py = srcH - 1;
    };

    for (int cy = 0; cy < rows_; ++cy) {
        int eff_y_top = (cy * 2 * effH) / totalPixelH;
        int eff_y_bot = ((cy * 2 + 1) * effH) / totalPixelH;
        if (eff_y_top >= effH) eff_y_top = effH - 1;
        if (eff_y_bot >= effH) eff_y_bot = effH - 1;

        for (int cx = 0; cx < cols_; ++cx) {
            int eff_x = (cx * effW) / cols_;
            if (eff_x >= effW) eff_x = effW - 1;

            int px_top = 0, py_top = 0;
            mapCoord(eff_x, eff_y_top, px_top, py_top);

            int px_bot = 0, py_bot = 0;
            mapCoord(eff_x, eff_y_bot, px_bot, py_bot);

            const uint8_t* ptr_top = src + py_top * stride + px_top * 3;
            const uint8_t* ptr_bot = src + py_bot * stride + px_bot * 3;

            CellColor& cell = outGrid[cy * cols_ + cx];

            if (isBgr) {
                cell.topB = ptr_top[0];
                cell.topG = ptr_top[1];
                cell.topR = ptr_top[2];

                cell.botB = ptr_bot[0];
                cell.botG = ptr_bot[1];
                cell.botR = ptr_bot[2];
            } else {
                cell.topR = ptr_top[0];
                cell.topG = ptr_top[1];
                cell.topB = ptr_top[2];

                cell.botR = ptr_bot[0];
                cell.botG = ptr_bot[1];
                cell.botB = ptr_bot[2];
            }
        }
    }
}

std::string Renderer::generateAnsiString(const uint8_t* data, int width, int height, int stride, bool isBgr) {
    if (!data || width <= 0 || height <= 0) {
        return "";
    }

    std::vector<CellColor> currGrid;
    buildCellGrid(data, width, height, stride, isBgr, currGrid);

    outputBuffer_.clear();
    outputBuffer_.reserve(cols_ * rows_ * 24);

    bool canDiff = config_.useDiff && !isFirstFrame_ && (prevGrid_.size() == currGrid.size());

    size_t changedCells = 0;
    if (canDiff) {
        for (size_t i = 0; i < currGrid.size(); ++i) {
            if (currGrid[i] != prevGrid_[i]) {
                changedCells++;
            }
        }
    }

    if (canDiff && changedCells == 0) {
        // No change, return cursor home and reset style
        outputBuffer_ += Terminal::cursorHome();
        return outputBuffer_;
    }

    // If diff is enabled and less than 40% of cells changed, update only changed cells
    if (canDiff && changedCells < (currGrid.size() * 4 / 10)) {
        int lastFgR = -1, lastFgG = -1, lastFgB = -1;
        int lastBgR = -1, lastBgG = -1, lastBgB = -1;

        for (int cy = 0; cy < rows_; ++cy) {
            for (int cx = 0; cx < cols_; ++cx) {
                int idx = cy * cols_ + cx;
                if (currGrid[idx] != prevGrid_[idx]) {
                    // Move cursor directly to cell (1-indexed: row, col)
                    outputBuffer_ += "\x1b[" + std::to_string(cy + 1) + ";" + std::to_string(cx + 1) + "H";

                    const auto& cell = currGrid[idx];
                    if (cell.topR != lastFgR || cell.topG != lastFgG || cell.topB != lastFgB) {
                        outputBuffer_ += Terminal::setFgRgb(cell.topR, cell.topG, cell.topB);
                        lastFgR = cell.topR; lastFgG = cell.topG; lastFgB = cell.topB;
                    }
                    if (cell.botR != lastBgR || cell.botG != lastBgG || cell.botB != lastBgB) {
                        outputBuffer_ += Terminal::setBgRgb(cell.botR, cell.botG, cell.botB);
                        lastBgR = cell.botR; lastBgG = cell.botG; lastBgB = cell.botB;
                    }
                    outputBuffer_ += Terminal::HALF_BLOCK;
                }
            }
        }
    } else {
        // Full frame sequential render
        outputBuffer_ += Terminal::cursorHome();

        int lastFgR = -1, lastFgG = -1, lastFgB = -1;
        int lastBgR = -1, lastBgG = -1, lastBgB = -1;

        for (int cy = 0; cy < rows_; ++cy) {
            for (int cx = 0; cx < cols_; ++cx) {
                int idx = cy * cols_ + cx;
                const auto& cell = currGrid[idx];

                if (cell.topR != lastFgR || cell.topG != lastFgG || cell.topB != lastFgB) {
                    outputBuffer_ += Terminal::setFgRgb(cell.topR, cell.topG, cell.topB);
                    lastFgR = cell.topR; lastFgG = cell.topG; lastFgB = cell.topB;
                }
                if (cell.botR != lastBgR || cell.botG != lastBgG || cell.botB != lastBgB) {
                    outputBuffer_ += Terminal::setBgRgb(cell.botR, cell.botG, cell.botB);
                    lastBgR = cell.botR; lastBgG = cell.botG; lastBgB = cell.botB;
                }
                outputBuffer_ += Terminal::HALF_BLOCK;
            }
            if (cy < rows_ - 1) {
                outputBuffer_ += "\n";
            }
        }
    }

    outputBuffer_ += Terminal::resetStyle();
    prevGrid_ = std::move(currGrid);
    isFirstFrame_ = false;

    return outputBuffer_;
}

void Renderer::renderRgb24(const uint8_t* rgb, int width, int height, int stride) {
    std::string ansi = generateAnsiString(rgb, width, height, stride, false);
    if (!ansi.empty()) {
        ssize_t ret = ::write(STDOUT_FILENO, ansi.data(), ansi.size());
        (void)ret;
    }
}

void Renderer::renderBgr24(const uint8_t* bgr, int width, int height, int stride) {
    std::string ansi = generateAnsiString(bgr, width, height, stride, true);
    if (!ansi.empty()) {
        ssize_t ret = ::write(STDOUT_FILENO, ansi.data(), ansi.size());
        (void)ret;
    }
}

} // namespace tcamviewer
