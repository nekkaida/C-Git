# Contributing to Mini Git

Thank you for your interest in contributing to Mini Git! This document provides guidelines and instructions for contributing.

## Code of Conduct

Please be respectful and considerate of others when contributing to this project.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR-USERNAME/mini-git.git`
3. Create a new branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Run tests: `make test`
6. Commit your changes: `git commit -am 'Add some feature'`
7. Push to the branch: `git push origin feature/your-feature-name`
8. Submit a pull request

## Development Environment

Make sure you have the following dependencies installed:

- GCC or compatible C compiler
- Make
- libcurl development headers
- zlib development headers

You can install these on Ubuntu/Debian with:

```
sudo apt-get install gcc make libcurl4-openssl-dev zlib1g-dev
```

## Coding Style

Please follow these guidelines for your code:

- Use 4 spaces for indentation (no tabs)
- Keep lines under 100 characters when possible
- Use descriptive variable and function names
- Add comments for complex logic
- Error handling should be robust and descriptive

## Testing

Add tests for new functionality. Run existing tests with:

```
make test
```

For memory leak checking:

```
make valgrind
```

## Pull Request Process

1. Update the README.md with details of significant changes
2. Update the CHANGELOG.md following the existing format
3. The PR should work on Linux, macOS, and Windows (if possible)
4. A maintainer will review and merge your PR

## Feature Requests and Bug Reports

Please use the GitHub issue tracker to report bugs or request features.