# u64OS 内核

中文简体 | [中文繁體](/docs/README_TW.md) | [English](/README.md)

---

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

> 一个从零开始实现的简单/实验性/教育型操作系统内核，专注于学习和研究操作系统原理。

## 📖 关于本项目

u64OS 是一个轻量级的操作系统内核，采用 C/C++ 编写，主要用于操作系统课程学习、内核开发实验和个人研究。目前支持 x86_64 架构，实现了 Multiboot2引导，临时启动tty，内存管理，中断，内核测试系统（实验性）。

本项目**欢迎用于个人学习、研究和私人修改**。任何分发（包括商业分发）必须遵守 GPLv3 许可证，并公开修改后的完整源代码。

## ✨ 特性

- 支持 x86_64架构
- 功能0：还没有什么实用的功能:(
- 简洁的代码结构，适合学习和二次开发

## 🚀 快速开始

### 前置要求
- 你需要的软件有：QEMU，CMake 3.22，GCC，xorriso，OVMF（若列举不详，欢迎通过Issue或者联系作者补充）
- 操作系统要求：Linux
- Python 3，用于辅助构建

### 构建与运行

```bash
# 克隆仓库
git clone https://github.com/xm-xyt03/u64OS.git
cd u64OS

# 构建内核
make
# 更快速的构建（推荐）
make -j$(nproc)

# 使用 QEMU 运行
make run
```

详细构建说明请参考 [构建文档](docs/build.md)。

## 📚 文档

- [安装与构建指南](./docs/build.md)
- [内核设计文档](./docs/design.md)
- [已实现功能列表](./docs/features.md)
- [TODO 与路线图](./docs/roadmap.md)

## 📜 许可证

本项目采用 **GNU General Public License v3.0 (GPLv3)** 发布。  
详见 [LICENSE](LICENSE) 文件。

- **允许**：个人学习、研究、私有修改和非商业使用。
- **要求**：任何分发修改版本必须以相同许可证公开完整源代码。
- **限制**：未经作者书面同意，**禁止**用于闭源商业用途（例如嵌入到专有操作系统或商业设备中销售）。

如果你希望将本内核用于闭源商业项目，请通过 Issues 或邮箱联系我，讨论许可事宜。

## 🤝 如何贡献

欢迎对本项目感兴趣的朋友提交 Issue 和 Pull Request！

贡献前请阅读 [CONTRIBUTING.md](./CONTRIBUTING.md)（仅限英文）。

主要贡献方式包括：
- 报告 Bug
- 提交新功能或优化
- 改进文档
- 代码审查

所有贡献者都需遵守 GPLv3 许可证。

## 🙋‍♂️ 作者

- [xm-xyt03的GitHub](https://github.com/xm-xyt03)
- 邮箱：xiaoyuntun073@gmail.com
- 备用邮箱：xiaoyuntun073@outlook.com
- X（Twitter）：[@xiaoyuntun073](https://x.com/xiaoyuntun073)

欢迎关注项目进展！

## ⭐ 支持本项目

如果你觉得这个项目对你学习操作系统有帮助，欢迎：
- 给仓库点个 Star ⭐
- Fork 并参与开发
- 在 Issues 中分享你的使用反馈或建议

---

**感谢你的兴趣与支持！**  
希望这个内核能帮助更多人更好地理解操作系统的工作原理。