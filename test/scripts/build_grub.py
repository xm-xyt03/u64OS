#!/usr/bin/python3
from ftplib import FTP
import threading
from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, ListView, ListItem, Label
from textual import on
from concurrent.futures import ThreadPoolExecutor, as_completed
import requests
import tempfile
import os
import tarfile
import subprocess
import shutil
import signal


class Colors:
    black, red, green, yellow, blue, pink, cyan, white = [
        f"\033[{i}m" for i in range(30, 38)
    ]
    lblack, lred, lgreen, lyellow, lblue, lpink, lcyan, lwhite = [
        f"\033[1;{i}m" for i in range(30, 38)
    ]
    reset = "\033[0m"


def list_ftp_files(server, directory="/gnu"):
    try:
        with FTP(server, timeout=30) as ftp:
            ftp.login()
            ftp.cwd(directory)
            files = ftp.nlst()
        return sorted(files)

    except Exception as e:
        raise LookupError(f"Error listing ftp server ({server})'s content: {e}")


class MenuPrompt(App):
    TITLE = "Select source of installing GRUB for u64OS"
    BINDINGS = [
        ("enter", "select", "Confirm Selection"),
    ]

    def compose(self) -> ComposeResult:
        yield Header()
        yield ListView(
            ListItem(Label("1. GNU FTP Server"), id="option0"),
            ListItem(
                Label(
                    "2. Tsinghua University GNU FTP Server Mirror (use in China Mainland)"
                ),
                id="option1",
            ),
            initial_index=0,
        )
        yield Footer()

    @on(ListView.Selected)
    def on_list_selected(self, event: ListView.Selected):
        opt = event.item.id
        self.exit(opt)


def detect_grub():

    grub_base = os.path.abspath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "grub")
    )
    if not os.path.exists(grub_base):
        print(
            "Error: GRUB is not builded. Please download and build GRUB with tests/scripts/grub_install.py"
        )
        exit(2)

    if len(os.listdir(grub_base)) == 1:
        print("GRUB is already installed!")
        exit()


def download_grub(server):
    """下载GRUB源代码包"""
    grub_dir = list_ftp_files(server, "/gnu/grub")
    grub_dir = [f for f in grub_dir if f.endswith(".tar.xz")]
    xz_file = grub_dir[-1]
    print(f"Downloading {xz_file}")
    fd, fname = tempfile.mkstemp(".tar.xz", "grub_")

    with requests.get(
        f"https://{server}/gnu/grub/{xz_file}", stream=True, timeout=30
    ) as response:
        response.raise_for_status()
        with open(fd, "wb") as f:
            for chunk in response.iter_content(chunk_size=8192):
                if chunk:
                    f.write(chunk)

    print(f"Successfully downloaded archive {xz_file} at {fname}")
    return fname, xz_file


def kill_process_tree(proc: subprocess.Popen, name: str, log_path: str):
    """强杀进程组（包括所有子进程），失败后尝试 SIGKILL"""
    if proc.poll() is not None:  # 已结束
        return

    try:
        pgid = os.getpgid(proc.pid)
        print(f"[{name}] Failed → Killing process group... (PGID={pgid}) ...")
        with open(log_path, "a", encoding="utf-8") as log:
            log.write(f"\n=== Failed → Kill process group (PGID={pgid}) ===\n")

        os.killpg(pgid, signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            print(f"[{name}] SIGTERM timeout → send SIGKILL")
            os.killpg(pgid, signal.SIGKILL)
            proc.wait(timeout=3)
    except (ProcessLookupError, OSError):
        # 进程已不存在或平台不支持 killpg（Windows）
        proc.kill()
        proc.wait()


def run_make_clean(cwd_path: str, name: str, log_path: str):
    """执行 make clean 并记录日志"""
    print(f"[{name}] Running make clean...")
    clean_log = (
        log_path.replace("_make.log", "_clean.log")
        if "_make.log" in log_path
        else f"{log_path}_clean.log"
    )

    with open(clean_log, "w", encoding="utf-8") as f:
        f.write(f"=== {name} make clean Log Start ===\n")
        try:
            result = subprocess.run(
                ["make", "clean"],
                cwd=cwd_path,
                stdout=f,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=30,
            )
            if result.returncode == 0:
                print(f"[{name}] make clean success")
            else:
                print(f"[{name}] make clean returns {result.returncode}")
        except Exception as e:
            print(f"[{name}] make clean execption: {e}")


def run_command_with_tee(cmd, cwd, log_path, name, stage, cancel_event, proc_ref=None):
    """实时输出 + 日志双写，支持返回 Popen 对象用于后续 kill"""
    if cancel_event.is_set():
        print(f"[{name}] {stage} Cancelled")
        return 1, None

    print(f"[{name}] {stage} Starting...")

    with open(log_path, "a", encoding="utf-8") as log:
        log.write(f"=== {name} {stage} Log Start ===\n")
        log.write(f"Command: {' '.join(map(str, cmd))}\n\n")
        log.flush()

        try:
            process = subprocess.Popen(
                cmd,
                cwd=cwd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                start_new_session=True,  # 关键：创建新进程组，便于 killpg
            )
            if proc_ref is not None:
                proc_ref[0] = process  # 保存引用用于后续 kill

            for line in iter(process.stdout.readline, ""):  # type: ignore
                if line:
                    stripped = line.rstrip("\n")
                    print(f"[{name}] {stripped}")
                    log.write(line)
                    log.flush()

            process.wait()
            return process.returncode, process

        except Exception as e:
            err = f"[{name}] {stage} Exception: {e}"
            print(err)
            with open(log_path, "a", encoding="utf-8") as log:
                log.write(f"\n{err}\n")
            return 1, None


def build_grub(archive_path, archive_name):
    """提取并编译GRUB"""

    def build_version(config, cancel_event):
        name = config["name"]

        cwd_path = os.path.join(grub_dir, config["dir"])
        log_prefix = os.path.join(grub_dir, name)

        make_proc_ref = [None]  # 用于保存 make 的 Popen 对象

        # 1. configure
        configure_cmd = [
            "../configure",
            f"--target={config['target']}",
            f"--with-platform={config['platform']}",
        ] + config["extra"]
        configure_log = f"{log_prefix}_configure.log"

        ret, _ = run_command_with_tee(
            configure_cmd, cwd_path, configure_log, name, "configure", cancel_event
        )
        if ret != 0:
            print(
                f"[{name}] configure {Colors.lred}failed{Colors.reset}! Log file: {configure_log}"
            )
            cancel_event.set()
            return name, False

        print(f"[{name}] configure {Colors.lgreen}success{Colors.reset}")

        # 2. make
        make_cmd = ["make", f"-j{nproc}"]
        make_log = f"{log_prefix}_make.log"

        ret, make_proc = run_command_with_tee(
            make_cmd, cwd_path, make_log, name, "make", cancel_event, make_proc_ref
        )

        if ret == 0:
            print(
                f"[{name}] make {Colors.lgreen}success{Colors.reset}! Log file: {make_log}"
            )
            return name, True
        else:
            print(f"[{name}] make failed! Return code: {ret}, Log file: {make_log}")

            # 失败处理：强杀 + clean
            if make_proc_ref[0] is not None:
                kill_process_tree(make_proc_ref[0], name, make_log)

            run_make_clean(cwd_path, name, make_log)

            cancel_event.set()
            return name, False

    output_dir = os.path.abspath(
        os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "..",
            "..",
            "grub",
        )
    )
    print(f"Extracting archive in {output_dir}")

    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    with tarfile.open(archive_path, mode="r:xz") as tar:
        tar.extractall(path=output_dir)

    os.remove(archive_path)

    grub_dir = os.path.join(output_dir, archive_name.removesuffix(".tar.xz"))

    print("Building GRUB...")
    for d in ["EFI32", "EFI64", "BIOS"]:
        os.makedirs(os.path.join(grub_dir, d), exist_ok=True)

    nproc = subprocess.check_output("nproc").decode("utf-8").strip()

    # 定义构建配置
    build_configs = [
        {
            "name": "EFI64",
            "dir": "EFI64",
            "target": "x86_64",
            "platform": "efi",
            "extra": [],
        },
        {
            "name": "EFI32",
            "dir": "EFI32",
            "target": "i386",
            "platform": "efi",
            "extra": [],
        },
        {
            "name": "BIOS",
            "dir": "BIOS",
            "target": "i386",
            "platform": "pc",
            "extra": ["--disable-nls"],
        },
    ]
    max_workers = 2
    cancel_event = threading.Event()
    success_count = 0

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        future_to_name = {
            executor.submit(build_version, cfg, cancel_event): cfg["name"]
            for cfg in build_configs
        }

        for future in as_completed(future_to_name):
            name, success = future.result()
            if success:
                success_count += 1
            else:
                if not cancel_event.is_set():
                    cancel_event.set()
                # 尝试取消未完成的任务
                for f in list(future_to_name.keys()):
                    if not f.done():
                        f.cancel()


detect_grub()


prompt = MenuPrompt()
option = int(prompt.run()[-1])  # type: ignore
servers = ["ftp.gnu.org", "mirrors.tuna.tsinghua.edu.cn"]

archive_path, archive_name = download_grub(servers[option])
build_grub(archive_path, archive_name)
