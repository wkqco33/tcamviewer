# Security Policy

The `tcamviewer` project takes security seriously. This document outlines how to report security vulnerabilities and our response process.

---

## 1. Supported Versions

| Version | Supported          |
| :---    | :---               |
| 0.1.x   | :white_check_mark: |
| < 0.1.0 | :x:                |

We actively support the latest minor release branch with security updates and critical bug fixes.

---

## 2. Reporting a Vulnerability

**Please do NOT report security vulnerabilities via public GitHub issues.**

If you believe you have found a security vulnerability in `tcamviewer`, please report it privately:

1. **GitHub Private Vulnerability Reporting**:
   Use GitHub's [Security Advisories](https://github.com/wkqco33/tcamviewer/security/advisories/new) feature on our repository.
2. **Email**:
   If private advisories are unavailable, send an email to the project maintainer with the subject `[SECURITY] tcamviewer vulnerability report`.

### What to Include in Your Report
To help us triage and resolve the issue quickly, please include:
- A detailed description of the vulnerability and potential impact.
- Step-by-step instructions to reproduce the issue (proof-of-concept code, sample input file, or network stream URL).
- Affected version(s), operating system, and architecture.
- Any suggested mitigations or patches, if available.

---

## 3. Response Process

- **Acknowledgment**: We aim to acknowledge receipt of your report within 48 hours.
- **Assessment**: The maintainers will evaluate the vulnerability and confirm its severity.
- **Fix & Disclosure**: Once a fix is verified and tested, a new release will be published. We follow coordinated disclosure and will credit the reporter in the release notes unless requested otherwise.
