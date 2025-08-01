# Contributing to XFCE4 NetworkManager Plugin

Thank you for your interest in contributing to the XFCE4 NetworkManager Plugin! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

By participating in this project, you agree to abide by our Code of Conduct:
- Be respectful and inclusive
- Welcome newcomers and help them get started
- Focus on constructive criticism
- Respect differing viewpoints and experiences

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in the [Issues](https://github.com/yourusername/xfce4-networkmanager-plugin/issues)
2. If not, create a new issue with:
   - Clear title and description
   - Steps to reproduce
   - Expected vs actual behavior
   - System information (XFCE version, distribution, NetworkManager version)
   - Relevant logs or error messages

### Suggesting Features

1. Check existing issues and pull requests
2. Open a new issue with the "enhancement" label
3. Describe the feature and its use case
4. Explain why it would benefit users

### Code Contributions

#### Setting Up Development Environment

1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/yourusername/xfce4-networkmanager-plugin.git
   cd xfce4-networkmanager-plugin
   ```

3. Install dependencies:
   ```bash
   # Debian/Ubuntu
   sudo apt install libglib2.0-dev libgtk-3-dev libxfce4panel-2.0-dev \
                    libxfce4ui-2-dev libnm-dev meson ninja-build
   ```

4. Build the project:
   ```bash
   meson setup build
   cd build
   ninja
   ```

#### Development Workflow

1. Create a new branch for your feature/fix:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. Make your changes following our coding standards

3. Test your changes:
   ```bash
   ninja test
   ```

4. Commit your changes:
   ```bash
   git add .
   git commit -m "Brief description of changes"
   ```

5. Push to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

6. Create a Pull Request

### Coding Standards

#### C Code Style

- Use 4 spaces for indentation (no tabs)
- Opening braces on new line for functions
- Opening braces on same line for control structures
- Maximum line length: 100 characters
- Use meaningful variable and function names

Example:
```c
void
function_name(gint parameter)
{
    if (condition) {
        /* Comment explaining why */
        do_something();
    }
}
```

#### Comments

- Use `/* */` for multi-line comments
- Use `//` sparingly for end-of-line comments
- Document all public functions
- Explain complex logic

#### Git Commits

- Write clear, concise commit messages
- Use present tense ("Add feature" not "Added feature")
- Reference issue numbers when applicable
- Keep commits focused on a single change

### Testing

- Write unit tests for new functionality
- Ensure all existing tests pass
- Test with different XFCE versions
- Verify memory leaks with Valgrind

### Documentation

- Update relevant documentation
- Add inline documentation for new functions
- Update README.md if adding new features
- Include examples where appropriate

## Pull Request Process

1. Ensure your PR:
   - Has a clear title and description
   - References any related issues
   - Includes tests for new functionality
   - Passes all CI checks

2. Be responsive to review comments
3. Keep your branch up to date with main
4. Squash commits if requested

## Getting Help

- Check the [Development Documentation](DEVELOPMENT.md)
- Ask questions in issues or discussions
- Join the XFCE development IRC/Matrix channels

## Recognition

Contributors will be recognized in:
- The AUTHORS file
- Release notes
- Project documentation

Thank you for contributing to XFCE4 NetworkManager Plugin!
