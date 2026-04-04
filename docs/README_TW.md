# u64OS 作業系統核心

中文繁體 | [中文简体](/docs/README_CN.md) | [English](/README.md)

---

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

> 一個從零開始實作的簡單／實驗性／教育型作業系統核心，專注於學習與研究作業系統原理。

## 📖 關於這個專案

u64OS 是一個輕量級的作業系統核心，使用 C/C++ 撰寫，主要用於作業系統課程學習、核心開發實驗與個人研究。目前支援 x86_64 架構，已實作 Multiboot2 開機載入器、啟動時期 TTY 、記憶體管理、中斷處理，以及核心測試系統（實驗性）。

這個專案**歡迎用於個人學習、研究與私人修改**。任何散布（包含商業用途）都必須遵守 GPLv3 授權，並公開修改後的完整程式碼。

## ✨ 特色

- 支援 x86_64 架構
- 功能0：還沒有什麼實用的功能 :(
- 程式碼結構簡潔清晰，適合學習與二次開發

## 🚀 快速開始

### 前置需求
- 你需要的軟體有：QEMU、CMake 3.22、GCC、xorriso、OVMF（如果列舉不完整，歡迎透過 Issue 或是聯絡作者補充）
- 作業系統需求：Linux
- Python 3，用來輔助建置

### 建置與執行

```bash
# 複製倉庫
git clone https://github.com/xm-xyt03/u64OS.git
cd u64OS

# 建置核心
make
# 更快速的建置（推薦）
make -j$(nproc)

# 使用 QEMU 執行
make run
```

詳細建置說明請參考 [建置文件](docs/build.md)。

## 📚 文件

- [安裝與建置指南](./docs/build.md)
- [核心設計文件](./docs/design.md)
- [已實作功能清單](./docs/features.md)
- [待辦事項與發展藍圖](./docs/roadmap.md)

## 📜 授權

本專案採用 **GNU General Public License v3.0 (GPLv3)** 釋出。  
詳細請見 [LICENSE](LICENSE) 檔案。

- **允許**：個人學習、研究、私人修改與非商業使用。
- **要求**：任何散布修改版本時，必須以相同授權公開完整程式碼。
- **限制**：未經作者書面同意，**禁止**用於閉源商業用途（例如嵌入專有作業系統或商業設備中銷售）。

如果你希望將這個核心用於閉源商業專案，請透過 Issues 或電子郵件聯絡我，討論授權事宜。

## 🤝 如何貢獻

歡迎對這個專案有興趣的朋友提交 Issue 和 Pull Request！

貢獻前請先閱讀 [CONTRIBUTING.md](./CONTRIBUTING.md)（僅限英文）。

主要貢獻方式包括：
- 回報 Bug
- 提交新功能或優化
- 改善文件
- 程式碼審核

所有貢獻者都必須遵守 GPLv3 授權。

## 🙋‍♂️ 作者

- [xm-xyt03 的 GitHub](https://github.com/xm-xyt03)
- 電子郵件：xiaoyuntun073@gmail.com
- 備用電子郵件：xiaoyuntun073@outlook.com
- X（Twitter）：[@xiaoyuntun073](https://x.com/xiaoyuntun073)

歡迎追蹤專案進度！

## ⭐ 支持這個專案

如果你覺得這個專案對你學習作業系統有幫助，歡迎：
- 給倉庫按個 Star ⭐
- Fork 並參與開發
- 在 Issues 中分享你的使用心得或建議

---

**感謝你的關注與支持！**  
希望這個核心能幫助更多人更深入理解作業系統的運作原理。