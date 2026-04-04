# u64OS Kernel

English | [中文简体](/docs/README_CN.md) | [中文繁體](/docs/README_TW.md)

---

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

> A from-scratch simple/experimental/educational operating system kernel, focused on learning and researching OS principles.

## 📖 About This Project

u64OS is a lightweight operating system kernel written in C/C++, primarily designed for OS course learning, kernel development experiments, and personal research. It currently supports the x86_64 architecture and has implemented Multiboot2 bootloader, temporary boot-stage TTY, memory management, interrupts, and a kernel test system (experimental).

This project **welcomes use for personal learning, research, and private modifications**. Any distribution (including commercial) must comply with the GPLv3 license and disclose the complete modified source code.

## ✨ Features

- Supports x86_64 architecture
- Feature 0: Nothing here :(
- Clean and simple code structure, suitable for learning and further development

## 🚀 Quick Start

### Prerequisites
- Required software: QEMU, CMake 3.22, GCC, xorriso, OVMF (if the list is incomplete, feel free to add via Issue or contact the author)
- OS requirement: Linux
- Python 3, used for auxiliary build scripts

### Build and Run

```bash
# Clone the repository
git clone https://github.com/xm-xyt03/u64OS.git
cd u64OS

# Build the kernel
make
# Faster build (recommended)
make -j$(nproc)

# Run with QEMU
make run
```

For detailed build instructions, see [Build Documentation](docs/build.md).

## 📚 Documentation

- [Installation and Build Guide](./docs/build.md)
- [Kernel Design Document](./docs/design.md)
- [Implemented Features List](./docs/features.md)
- [TODO and Roadmap](./docs/roadmap.md)

## 📜 License

This project is released under the **GNU General Public License v3.0 (GPLv3)**.  
See the [LICENSE](LICENSE) file for details.

- **Allowed**: Personal learning, research, private modifications, and non-commercial use.
- **Required**: Any distributed modified version must be released under the same license with complete source code.
- **Restricted**: Without the author's written consent, **prohibited** from use in closed-source commercial products (e.g., embedding into proprietary operating systems or commercial devices for sale).

If you wish to use this kernel in a closed-source commercial project, please contact me via Issues or email to discuss licensing.

## 🤝 How to Contribute

Contributions from anyone interested in this project are welcome via Issues and Pull Requests!

Please read [CONTRIBUTING.md](./CONTRIBUTING.md) before contributing.

Main ways to contribute:
- Report bugs
- Submit new features or optimizations
- Improve documentation
- Code review

All contributors must comply with the GPLv3 license.

## 🙋‍♂️ Author

- [xm-xyt03's GitHub](https://github.com/xm-xyt03)
- Email: xiaoyuntun073@gmail.com
- Alternate Email: xiaoyuntun073@outlook.com
- X (Twitter): [@xiaoyuntun073](https://x.com/xiaoyuntun073)

Feel free to follow the project for updates!

## ⭐ Support This Project

If you find this project helpful for learning operating systems, please:
- Star the repository ⭐
- Fork and participate in development
- Share your feedback or suggestions in Issues

---

**Thank you for your interest and support!**  
Hope this kernel helps more people better understand how operating systems work.