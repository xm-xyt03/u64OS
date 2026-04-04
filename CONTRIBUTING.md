# Contributing to u64OS

Thank you for your interest in contributing to u64OS!  
We welcome contributions from the community to help improve this educational operating system kernel.

## 📋 Before You Start

- This project is licensed under **GPLv3**. All contributions must comply with the GPLv3 license.
- The project is currently focused on learning and experimental purposes.
- Please read this document before submitting to avoid duplicate work or guideline violations.

## 🚀 How to Contribute

You can contribute in the following ways:

- Submit **Issues** (report bugs, suggest features, or share ideas)
- Submit **Pull Requests** (fix bugs, add features, improve documentation)
- Improve documentation and comments
- Provide testing feedback

### 1. Submitting Issues

Before creating a new Issue, please check if a similar one already exists.

Good Issue types include:
- Bug reports (please include reproduction steps, error messages, and environment details)
- Feature suggestions (explain why the feature fits an educational kernel)
- Documentation improvements
- Build or runtime problems

### 2. Submitting Pull Requests

**Steps:**

1. Fork the repository
2. Create a new branch with a meaningful name (e.g. `fix-memory-leak` or `add-interrupt-handling`)
3. Make your changes and test locally
4. Commit your changes with clear messages
5. Push the branch and open a Pull Request

#### Pull Request Requirements

- Use a clear and descriptive PR title (e.g. "fix: resolve null pointer issue in memory management")
- In the PR description, please include:
  - What problem this solves
  - What changes were made
  - How it was tested
- Keep code style consistent with the existing codebase (indentation, naming conventions, etc.)
- Add appropriate comments for new or modified logic
- For larger changes, it is recommended to open an Issue first for discussion

## 🛠 Development Environment Setup

Please refer to the "Quick Start" section in the [README.md](./README.md) to set up QEMU, CMake, GCC, and other tools.

Build commands:
```bash
make -j$(nproc)   # Recommended for faster build
make run
```

## 📝 Code Guidelines

- Follow the existing code style (currently using 4 spaces indentation)
- Use clear and meaningful names for functions and variables
- Add comments for important logic
- Avoid introducing unnecessary external dependencies (the project aims to stay lightweight)

## ❓ Frequently Asked Questions

**Q: Can I submit a large new feature directly?**  
A: For significant changes, please open an Issue first to discuss. This helps avoid duplicated effort or misaligned direction.

**Q: The project doesn't have many features yet. What can I do?**  
A: You're very welcome! You can:
- Improve comments and documentation
- Add test cases
- Optimize the build process
- Fix known issues (check the Issues tab)

## 💬 Get in Touch

- GitHub Issues (preferred)
- Email: xiaoyuntun073@gmail.com
- Alternate Email: xiaoyuntun073@outlook.com
- X (Twitter): [@xiaoyuntun073](https://x.com/xiaoyuntun073)

---

**Thank you for your contributions!**  
Every bit of code, documentation, or feedback helps make u64OS better and helps more people learn about operating systems.

We look forward to your Pull Requests! 🚀