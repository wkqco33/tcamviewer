# Contributing to tcamviewer

Thank you for your interest in contributing to `tcamviewer`! We welcome all contributions, including bug reports, documentation enhancements, feature proposals, and pull requests.

---

## 1. Code of Conduct

We are committed to providing a welcoming, inclusive, and harassment-free experience for everyone. Please be respectful and constructive in discussions, reviews, and interactions.

---

## 2. Getting Started & Development Setup

### Prerequisites
- **Linux** (Ubuntu 22.04 or 24.04 recommended)
- **C++17** compatible compiler (`g++` or `clang++`)
- **CMake** (>= 3.16)
- **Task** ([go-task](https://taskfile.dev))
- **FFmpeg development libraries**: `libavcodec-dev`, `libavformat-dev`, `libswscale-dev`, `libavutil-dev`
- **GoogleTest**: `libgtest-dev`
- *(Optional)* Go 1.22+, Python 3.10+, ROS 2 Jazzy/Humble

Install dependencies on Ubuntu/Debian:
```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake pkg-config libgtest-dev \
    libavcodec-dev libavformat-dev libswscale-dev libavutil-dev
```

### Building & Testing
We use `Taskfile.yml` for unified development workflow:

```bash
# Build core C++ library, CLI, and tests
task build

# Run all test suites (C++ GoogleTest, Go, Python)
task test

# Run C++ tests only
task test:cpp

# Run Go tests only
task test:go

# Run Python tests only
task test:py
```

---

## 3. Development Workflow

1. **Fork and Clone** the repository:
   ```bash
   git clone https://github.com/<your-username>/tcamviewer.git
   cd tcamviewer
   git submodule update --init --recursive
   ```
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feat/your-feature-name
   ```
3. **Follow Test-Driven Development (TDD)**:
   - Always write or update unit tests in `tests/`, `pkg/tcamviewer/`, or `python/tests/` when modifying or adding functionality.
   - Verify that all tests pass (`task test`) before opening a pull request.
4. **Adhere to Code Styles**:
   - **C++**: Follow the `.clang-format` configuration (Google C++ style, 4 spaces indent).
   - **Go**: Ensure `go fmt ./...` and `go vet ./...` produce no warnings.
   - **Python**: Follow PEP 8 guidelines.
5. **Commit Your Changes**:
   Follow [Conventional Commits](https://www.conventionalcommits.org/):
   - `feat: ...` for new features
   - `fix: ...` for bug fixes
   - `perf: ...` for performance improvements
   - `refactor: ...` for code refactoring
   - `docs: ...` for documentation changes
   - `test: ...` for adding or updating tests
6. **Submit a Pull Request**:
   - Open a PR against the `master` branch.
   - Fill in the PR description template clearly detailing changes, motivations, and test results.

---

## 4. Reporting Issues

- **Bug Reports**: Use the GitHub Issue Tracker. Provide clear reproduction steps, environment details (OS, terminal emulator, resolution), and error logs.
- **Feature Requests**: Describe the problem and the proposed solution, along with potential alternatives.
- **Security Issues**: Please refer to [SECURITY.md](SECURITY.md) for private vulnerability disclosure guidelines.
