#include "tcamviewer/renderer.hpp"
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>

namespace tcamviewer {

namespace {

inline void appendUint(std::string& out, unsigned int val) {
    char buf[12];
    char* p = buf + sizeof(buf);
    do {
        *--p = static_cast<char>('0' + (val % 10));
        val /= 10;
    } while (val > 0);
    out.append(p, buf + sizeof(buf) - p);
}

inline void appendUint8(std::string& out, uint8_t v) {
    if (v >= 100) {
        out.push_back(static_cast<char>('0' + v / 100));
        out.push_back(static_cast<char>('0' + (v / 10) % 10));
        out.push_back(static_cast<char>('0' + v % 10));
    } else if (v >= 10) {
        out.push_back(static_cast<char>('0' + v / 10));
        out.push_back(static_cast<char>('0' + v % 10));
    } else {
        out.push_back(static_cast<char>('0' + v));
    }
}

inline void appendFgRgb(std::string& out, uint8_t r, uint8_t g, uint8_t b) {
    out.append("\x1b[38;2;");
    appendUint8(out, r);
    out.push_back(';');
    appendUint8(out, g);
    out.push_back(';');
    appendUint8(out, b);
    out.push_back('m');
}

inline void appendBgRgb(std::string& out, uint8_t r, uint8_t g, uint8_t b) {
    out.append("\x1b[48;2;");
    appendUint8(out, r);
    out.push_back(';');
    appendUint8(out, g);
    out.push_back(';');
    appendUint8(out, b);
    out.push_back('m');
}

inline void appendCursorMove(std::string& out, int col, int row) {
    out.append("\x1b[");
    appendUint(out, static_cast<unsigned int>(row));
    out.push_back(';');
    appendUint(out, static_cast<unsigned int>(col));
    out.push_back('H');
}

} // anonymous namespace

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
    currGrid_.clear();
    isFirstFrame_ = true;
}

void Renderer::setRotation(int degrees) {
    int rot = (degrees % 360 + 360) % 360;
    if (config_.rotation != rot) {
        config_.rotation = rot;
        invalidateCache();
    }
}

void Renderer::setKeepAspectRatio(bool enable) {
    if (config_.keepAspectRatio != enable) {
        config_.keepAspectRatio = enable;
        invalidateCache();
    }
}

void Renderer::buildCellGrid(const uint8_t* src, int srcW, int srcH, int stride, bool isBgr,
                             std::vector<CellColor>& outGrid) {
    size_t totalCells = static_cast<size_t>(cols_) * rows_;
    if (outGrid.size() != totalCells) {
        outGrid.resize(totalCells);
    }
    std::memset(outGrid.data(), 0, totalCells * sizeof(CellColor));

    if (!src || srcW <= 0 || srcH <= 0) return;
    if (stride <= 0) stride = srcW * 3;

    int rot = (config_.rotation % 360 + 360) % 360;
    int effW = (rot == 90 || rot == 270) ? srcH : srcW;
    int effH = (rot == 90 || rot == 270) ? srcW : srcH;

    int renderCols = cols_;
    int renderRows = rows_;
    int offsetCol = 0;
    int offsetRow = 0;

    if (config_.keepAspectRatio && effW > 0 && effH > 0) {
        double srcAspect = static_cast<double>(effW) / effH;
        int availPixelW = cols_;
        int availPixelH = rows_ * 2;
        double termAspect = static_cast<double>(availPixelW) / availPixelH;

        int fitPixelW = availPixelW;
        int fitPixelH = availPixelH;

        if (termAspect > srcAspect) {
            fitPixelH = availPixelH;
            fitPixelW = static_cast<int>(std::round(fitPixelH * srcAspect));
            if (fitPixelW > availPixelW) fitPixelW = availPixelW;
        } else {
            fitPixelW = availPixelW;
            fitPixelH = static_cast<int>(std::round(fitPixelW / srcAspect));
            if (fitPixelH > availPixelH) fitPixelH = availPixelH;
        }

        if (fitPixelW < 1) fitPixelW = 1;
        if (fitPixelH < 1) fitPixelH = 1;

        renderCols = fitPixelW;
        renderRows = (fitPixelH + 1) / 2;
        if (renderCols > cols_) renderCols = cols_;
        if (renderRows > rows_) renderRows = rows_;

        offsetCol = (cols_ - renderCols) / 2;
        offsetRow = (rows_ - renderRows) / 2;
    }

    std::vector<int> eff_x_lut(renderCols);
    for (int c = 0; c < renderCols; ++c) {
        int ex = (c * effW) / renderCols;
        eff_x_lut[c] = (ex >= effW) ? effW - 1 : (ex < 0 ? 0 : ex);
    }

    for (int r = 0; r < renderRows; ++r) {
        int cy = offsetRow + r;
        if (cy >= rows_) break;

        int eff_y_top = (r * 2 * effH) / (renderRows * 2);
        int eff_y_bot = ((r * 2 + 1) * effH) / (renderRows * 2);
        if (eff_y_top >= effH) eff_y_top = effH - 1;
        if (eff_y_bot >= effH) eff_y_bot = effH - 1;

        CellColor* rowCells = &outGrid[cy * cols_ + offsetCol];

        for (int c = 0; c < renderCols; ++c) {
            int eff_x = eff_x_lut[c];
            int px_top = 0, py_top = 0;
            int px_bot = 0, py_bot = 0;

            if (rot == 0) {
                px_top = eff_x; py_top = eff_y_top;
                px_bot = eff_x; py_bot = eff_y_bot;
            } else if (rot == 90) {
                px_top = eff_y_top; py_top = srcH - 1 - eff_x;
                px_bot = eff_y_bot; py_bot = srcH - 1 - eff_x;
            } else if (rot == 180) {
                px_top = srcW - 1 - eff_x; py_top = srcH - 1 - eff_y_top;
                px_bot = srcW - 1 - eff_x; py_bot = srcH - 1 - eff_y_bot;
            } else { // 270
                px_top = srcW - 1 - eff_y_top; py_top = eff_x;
                px_bot = srcW - 1 - eff_y_bot; py_bot = eff_x;
            }

            if (px_top < 0) px_top = 0; else if (px_top >= srcW) px_top = srcW - 1;
            if (py_top < 0) py_top = 0; else if (py_top >= srcH) py_top = srcH - 1;
            if (px_bot < 0) px_bot = 0; else if (px_bot >= srcW) px_bot = srcW - 1;
            if (py_bot < 0) py_bot = 0; else if (py_bot >= srcH) py_bot = srcH - 1;

            const uint8_t* ptr_top = src + py_top * stride + px_top * 3;
            const uint8_t* ptr_bot = src + py_bot * stride + px_bot * 3;

            CellColor& cell = rowCells[c];
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

void Renderer::generateAnsiInternal(const uint8_t* data, int width, int height, int stride, bool isBgr) {
    if (!data || width <= 0 || height <= 0) {
        outputBuffer_.clear();
        return;
    }

    buildCellGrid(data, width, height, stride, isBgr, currGrid_);

    outputBuffer_.clear();
    outputBuffer_.reserve(static_cast<size_t>(cols_) * rows_ * 24);

    bool canDiff = config_.useDiff && !isFirstFrame_ && (prevGrid_.size() == currGrid_.size());

    size_t changedCells = 0;
    if (canDiff) {
        const CellColor* currPtr = currGrid_.data();
        const CellColor* prevPtr = prevGrid_.data();
        size_t total = currGrid_.size();
        for (size_t i = 0; i < total; ++i) {
            if (currPtr[i] != prevPtr[i]) {
                changedCells++;
            }
        }
    }

    if (canDiff && changedCells == 0) {
        outputBuffer_.append(Terminal::cursorHome());
        return;
    }

    if (canDiff && changedCells < (currGrid_.size() * 4 / 10)) {
        int lastFgR = -1, lastFgG = -1, lastFgB = -1;
        int lastBgR = -1, lastBgG = -1, lastBgB = -1;

        for (int cy = 0; cy < rows_; ++cy) {
            for (int cx = 0; cx < cols_; ++cx) {
                int idx = cy * cols_ + cx;
                if (currGrid_[idx] != prevGrid_[idx]) {
                    appendCursorMove(outputBuffer_, cx + 1, cy + 1);

                    const auto& cell = currGrid_[idx];
                    if (cell.topR != lastFgR || cell.topG != lastFgG || cell.topB != lastFgB) {
                        appendFgRgb(outputBuffer_, cell.topR, cell.topG, cell.topB);
                        lastFgR = cell.topR; lastFgG = cell.topG; lastFgB = cell.topB;
                    }
                    if (cell.botR != lastBgR || cell.botG != lastBgG || cell.botB != lastBgB) {
                        appendBgRgb(outputBuffer_, cell.botR, cell.botG, cell.botB);
                        lastBgR = cell.botR; lastBgG = cell.botG; lastBgB = cell.botB;
                    }
                    outputBuffer_.append(Terminal::HALF_BLOCK);
                }
            }
        }
    } else {
        outputBuffer_.append(Terminal::cursorHome());

        int lastFgR = -1, lastFgG = -1, lastFgB = -1;
        int lastBgR = -1, lastBgG = -1, lastBgB = -1;

        for (int cy = 0; cy < rows_; ++cy) {
            for (int cx = 0; cx < cols_; ++cx) {
                int idx = cy * cols_ + cx;
                const auto& cell = currGrid_[idx];

                if (cell.topR != lastFgR || cell.topG != lastFgG || cell.topB != lastFgB) {
                    appendFgRgb(outputBuffer_, cell.topR, cell.topG, cell.topB);
                    lastFgR = cell.topR; lastFgG = cell.topG; lastFgB = cell.topB;
                }
                if (cell.botR != lastBgR || cell.botG != lastBgG || cell.botB != lastBgB) {
                    appendBgRgb(outputBuffer_, cell.botR, cell.botG, cell.botB);
                    lastBgR = cell.botR; lastBgG = cell.botG; lastBgB = cell.botB;
                }
                outputBuffer_.append(Terminal::HALF_BLOCK);
            }
            if (cy < rows_ - 1) {
                outputBuffer_.push_back('\n');
            }
        }
    }

    outputBuffer_.append(Terminal::resetStyle());
    prevGrid_ = currGrid_;
    isFirstFrame_ = false;
}

std::string Renderer::generateAnsiString(const uint8_t* data, int width, int height, int stride, bool isBgr) {
    generateAnsiInternal(data, width, height, stride, isBgr);
    return outputBuffer_;
}

void Renderer::renderRgb24(const uint8_t* rgb, int width, int height, int stride) {
    generateAnsiInternal(rgb, width, height, stride, false);
    if (!outputBuffer_.empty()) {
        ssize_t ret = ::write(STDOUT_FILENO, outputBuffer_.data(), outputBuffer_.size());
        (void)ret;
    }
}

void Renderer::renderBgr24(const uint8_t* bgr, int width, int height, int stride) {
    generateAnsiInternal(bgr, width, height, stride, true);
    if (!outputBuffer_.empty()) {
        ssize_t ret = ::write(STDOUT_FILENO, outputBuffer_.data(), outputBuffer_.size());
        (void)ret;
    }
}

} // namespace tcamviewer
