# Build u64OS | 编译u64OS

## Install Dependencies 安装依赖
Operating System 操作系统：Linux
```bash
# Install Command 安装命令（Ubuntu/Debian）
sudo apt upgrade
sudo apt update
sudo apt install -y qemu gcc cmake git make python3 ovmf
```

## Build 构建
```bash
# Fast Startup 快速启动
make -j$(nproc)
make run
```

The packed ISO file is in **target/kernel.iso**

打包好的 ISO 镜像在 **target/kernel.iso**
