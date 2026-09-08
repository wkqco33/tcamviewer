#include "wcppcli/wcli.hpp"
#include "wcppcli/wlog.hpp"
#include "tcamviewer/terminal.hpp"
#include "tcamviewer/renderer.hpp"
#include "tcamviewer/decoder.hpp"

#include <csignal>
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace wcppcli;
using namespace tcamviewer;

static volatile std::sig_atomic_t g_running = 1;
static volatile std::sig_atomic_t g_resized = 0;

static void sigHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_running = 0;
    } else if (sig == SIGWINCH) {
        g_resized = 1;
    }
}

struct TerminalInputGuard {
    struct termios orig_termios;
    bool active{false};

    TerminalInputGuard() {
        if (isatty(STDIN_FILENO)) {
            tcgetattr(STDIN_FILENO, &orig_termios);
            struct termios raw = orig_termios;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            active = true;
        }
    }
    ~TerminalInputGuard() {
        if (active) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        }
    }
};

static int runTestPattern(int durationSec, int targetFps) {
    std::signal(SIGINT, sigHandler);
    std::signal(SIGWINCH, sigHandler);

    RenderConfig cfg;
    cfg.useDiff = true;
    cfg.altScreen = true;
    cfg.hideCursor = true;

    Renderer renderer(cfg);

    int fps = targetFps > 0 ? targetFps : 30;
    auto frameInterval = std::chrono::microseconds(1000000 / fps);
    int totalFrames = durationSec * fps;

    WLog::info("Starting terminal video test pattern...");

    int frameCount = 0;
    auto startTime = std::chrono::steady_clock::now();

    while (g_running && (totalFrames <= 0 || frameCount < totalFrames)) {
        auto frameStart = std::chrono::steady_clock::now();

        if (g_resized) {
            g_resized = 0;
            renderer.resize(0, 0); // auto-detect new terminal size
        }

        int cols = renderer.getCols();
        int rows = renderer.getRows();
        int pixelW = cols;
        int pixelH = rows * 2;

        std::vector<uint8_t> rgb(pixelW * pixelH * 3);
        float t = frameCount * 0.05f;

        for (int y = 0; y < pixelH; ++y) {
            for (int x = 0; x < pixelW; ++x) {
                float nx = static_cast<float>(x) / pixelW;
                float ny = static_cast<float>(y) / pixelH;

                uint8_t r = static_cast<uint8_t>((std::sin(nx * 3.1415f * 2.0f + t) * 0.5f + 0.5f) * 255.0f);
                uint8_t g = static_cast<uint8_t>((std::sin(ny * 3.1415f * 2.0f + t * 1.5f) * 0.5f + 0.5f) * 255.0f);
                uint8_t b = static_cast<uint8_t>((std::cos((nx + ny) * 3.1415f + t * 0.7f) * 0.5f + 0.5f) * 255.0f);

                int idx = (y * pixelW + x) * 3;
                rgb[idx + 0] = r;
                rgb[idx + 1] = g;
                rgb[idx + 2] = b;
            }
        }

        renderer.renderRgb24(rgb.data(), pixelW, pixelH, pixelW * 3);
        frameCount++;

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - frameStart
        );
        if (elapsed < frameInterval) {
            std::this_thread::sleep_for(frameInterval - elapsed);
        }
    }

    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime
    ).count();

    double actualFps = totalTime > 0 ? (frameCount * 1000.0 / totalTime) : 0.0;
    WLog::success("Test completed: rendered " + std::to_string(frameCount) +
                  " frames, avg " + std::to_string(static_cast<int>(actualFps)) + " FPS");

    return 0;
}

static int runPlayVideo(const std::string& source, bool loop, bool noDiff, bool altScreen, int targetW, int targetH, int rotateDeg, bool stretch) {
    std::signal(SIGINT, sigHandler);
    std::signal(SIGWINCH, sigHandler);

    TerminalInputGuard inputGuard;

    VideoDecoder decoder(source, loop);
    if (!decoder.open()) {
        WLog::error("Failed to open video source: " + source);
        return 1;
    }

    const auto& info = decoder.getInfo();
    int initialRotation = (rotateDeg >= 0) ? rotateDeg : info.rotation;
    WLog::info("Opened source: " + source + " (" + std::to_string(info.width) + "x" +
               std::to_string(info.height) + " @" + std::to_string(static_cast<int>(info.fps)) + "fps, rotate=" +
               std::to_string(initialRotation) + "deg, aspect_fit=" + (stretch ? "false" : "true") + ")");

    RenderConfig cfg;
    cfg.targetCols = targetW;
    cfg.targetRows = targetH;
    cfg.useDiff = !noDiff;
    cfg.altScreen = altScreen;
    cfg.hideCursor = true;
    cfg.rotation = initialRotation;
    cfg.keepAspectRatio = !stretch;

    Renderer renderer(cfg);

    double fps = (info.fps > 0.0 && info.fps <= 120.0) ? info.fps : 30.0;
    auto frameInterval = std::chrono::microseconds(static_cast<long long>(1000000.0 / fps));

    while (g_running) {
        auto frameStart = std::chrono::steady_clock::now();

        // Check for interactive keypresses (non-blocking)
        char ch = 0;
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'q' || ch == 'Q') {
                break;
            } else if (ch == 'r' || ch == 'R') {
                int nextRot = (renderer.getRotation() + 90) % 360;
                renderer.setRotation(nextRot);
            } else if (ch == 'a' || ch == 'A') {
                renderer.setKeepAspectRatio(!renderer.isKeepAspectRatio());
            }
        }

        if (g_resized) {
            g_resized = 0;
            renderer.resize(targetW, targetH);
        }

        int availW = renderer.getCols();
        int availH = renderer.getRows() * 2;
        int targetW_dec = availW;
        int targetH_dec = availH;

        if (renderer.isKeepAspectRatio() && info.width > 0 && info.height > 0) {
            int rot = renderer.getRotation();
            int effW = (rot == 90 || rot == 270) ? info.height : info.width;
            int effH = (rot == 90 || rot == 270) ? info.width : info.height;
            double aspect = static_cast<double>(effW) / effH;
            if ((static_cast<double>(availW) / availH) > aspect) {
                int fitH = availH;
                int fitW = std::max(1, static_cast<int>(std::round(fitH * aspect)));
                targetW_dec = (rot == 90 || rot == 270) ? fitH : fitW;
                targetH_dec = (rot == 90 || rot == 270) ? fitW : fitH;
            } else {
                int fitW = availW;
                int fitH = std::max(1, static_cast<int>(std::round(fitW / aspect)));
                targetW_dec = (rot == 90 || rot == 270) ? fitH : fitW;
                targetH_dec = (rot == 90 || rot == 270) ? fitW : fitH;
            }
        }

        int outW = 0, outH = 0, outStride = 0;
        const uint8_t* frameData = decoder.readFrame(targetW_dec, targetH_dec, &outW, &outH, &outStride);
        if (!frameData) {
            break; // EOF or error
        }

        renderer.renderRgb24(frameData, outW, outH, outStride);

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - frameStart
        );
        if (elapsed < frameInterval) {
            std::this_thread::sleep_for(frameInterval - elapsed);
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    Command root;
    root.name = "tcamviewer";
    root.description = "High-performance Terminal Video Player & ROS2 Camera Monitor";
    root.version = "0.1.0";

    // Play subcommand
    auto playCmd = std::make_unique<Command>();
    playCmd->name = "play";
    playCmd->description = "Play a video file, network stream (RTSP/RTMP), or V4L2 webcam";
    playCmd->usage = "tcamviewer play <source> [flags]";

    bool loop = false;
    bool noDiff = false;
    bool altScreen = false;
    int targetW = 0;
    int targetH = 0;

    Flag loopFlag;
    loopFlag.name = "loop";
    loopFlag.shorthand = 'l';
    loopFlag.description = "Loop video playback continuously";
    loopFlag.value_ptr = &loop;
    playCmd->add_flag(loopFlag);

    Flag noDiffFlag;
    noDiffFlag.name = "no-diff";
    noDiffFlag.description = "Disable dirty-diff optimization";
    noDiffFlag.value_ptr = &noDiff;
    playCmd->add_flag(noDiffFlag);

    Flag altScreenFlag;
    altScreenFlag.name = "alt-screen";
    altScreenFlag.description = "Use terminal alternate screen buffer";
    altScreenFlag.value_ptr = &altScreen;
    playCmd->add_flag(altScreenFlag);

    Flag widthFlag;
    widthFlag.name = "width";
    widthFlag.shorthand = 'w';
    widthFlag.description = "Explicit terminal width in columns";
    widthFlag.value_ptr = &targetW;
    playCmd->add_flag(widthFlag);

    Flag heightFlag;
    heightFlag.name = "height";
    heightFlag.shorthand = 'h';
    heightFlag.description = "Explicit terminal height in rows";
    heightFlag.value_ptr = &targetH;
    playCmd->add_flag(heightFlag);

    int rotateDeg = -1; // -1 = auto detect
    Flag rotateFlag;
    rotateFlag.name = "rotate";
    rotateFlag.shorthand = 'r';
    rotateFlag.description = "Rotation in degrees: 0, 90, 180, 270 (default: auto from metadata)";
    rotateFlag.value_ptr = &rotateDeg;
    playCmd->add_flag(rotateFlag);

    bool stretch = false;
    Flag stretchFlag;
    stretchFlag.name = "stretch";
    stretchFlag.description = "Stretch video to fill terminal, ignoring aspect ratio";
    stretchFlag.value_ptr = &stretch;
    playCmd->add_flag(stretchFlag);

    playCmd->handler = [&](const Command& cmd) -> int {
        if (cmd.args.empty()) {
            WLog::error("Please specify a video file, stream URL, or webcam device (e.g. /dev/video0)");
            return 1;
        }
        std::string source = cmd.args[0];
        return runPlayVideo(source, loop, noDiff, altScreen, targetW, targetH, rotateDeg, stretch);
    };

    // Test subcommand
    auto testCmd = std::make_unique<Command>();
    testCmd->name = "test";
    testCmd->description = "Run a terminal video test pattern to verify TrueColor and FPS";
    testCmd->usage = "tcamviewer test [flags]";

    int duration = 5;
    int testFps = 30;

    Flag durFlag;
    durFlag.name = "duration";
    durFlag.shorthand = 'd';
    durFlag.description = "Duration in seconds (default: 5)";
    durFlag.value_ptr = &duration;
    testCmd->add_flag(durFlag);

    Flag fpsFlag;
    fpsFlag.name = "fps";
    fpsFlag.description = "Target FPS (default: 30)";
    fpsFlag.value_ptr = &testFps;
    testCmd->add_flag(fpsFlag);

    testCmd->handler = [&](const Command&) -> int {
        return runTestPattern(duration, testFps);
    };

    root.add_command(std::move(playCmd));
    root.add_command(std::move(testCmd));

    return root.execute(argc, argv);
}
