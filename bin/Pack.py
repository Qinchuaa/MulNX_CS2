#!/usr/bin/env python3
"""
MulNX项目打包脚本
功能：
1. 检查Output文件夹是否存在，不存在则报错
2. 清空Output文件夹
3. 复制MulNX文件夹到Output，忽略所有.pdb文件
4. 将Output中的MulNX文件夹压缩为MulNX.zip
"""

import os
import shutil
import zipfile
import sys
import io
from pathlib import Path
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')


def clear_output_folder(output_path):
    """清空Output文件夹"""
    if output_path.exists() and output_path.is_dir():
        print(f"正在清空Output文件夹: {output_path}")
        for item in output_path.iterdir():
            if item.is_file() or item.is_symlink():
                item.unlink()
            elif item.is_dir():
                shutil.rmtree(item)
        print("  ✓ Output文件夹已清空")
    else:
        print(f"  ! Output文件夹不存在或不是目录: {output_path}")


def copy_MulNX_without_pdb(source_dir, output_dir):
    """
    复制MulNX文件夹到Output，忽略.pdb文件
    """
    print(f"正在复制MulNX文件夹: {source_dir} -> {output_dir}")

    MulNX_output_dir = output_dir / "MulNX"
    MulNX_output_dir.mkdir(parents=True, exist_ok=True)

    copied_files = 0
    skipped_files = 0
    copied_dirs = 0

    for root, dirs, files in os.walk(source_dir):
        rel_path = Path(root).relative_to(source_dir)
        target_root = MulNX_output_dir / rel_path
        target_root.mkdir(parents=True, exist_ok=True)
        copied_dirs += 1

        for file in files:
            source_file = Path(root) / file
            target_file = target_root / file

            if file.lower().endswith('.pdb'):
                skipped_files += 1
                continue

            shutil.copy2(source_file, target_file)
            copied_files += 1

    print(f"  ✓ 复制完成: 创建{copied_dirs}个目录, 复制{copied_files}个文件, 跳过{skipped_files}个.pdb文件")
    return MulNX_output_dir


def zip_MulNX_folder(MulNX_folder, output_zip_path):
    """将MulNX文件夹压缩为MulNX.zip，包含空目录"""
    print(f"正在压缩MulNX文件夹: {MulNX_folder} -> {output_zip_path}")

    output_zip_path.parent.mkdir(parents=True, exist_ok=True)
    MulNX_parent = MulNX_folder.parent

    with zipfile.ZipFile(output_zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        # 先添加所有空目录
        print("  添加目录结构...")
        empty_dirs_added = 0
        for root, dirs, files in os.walk(MulNX_folder):
            dir_rel_path = Path(root).relative_to(MulNX_parent)
            if not os.listdir(root):  # 空目录
                zip_dir_path = str(dir_rel_path).replace('\\', '/') + '/'
                if zip_dir_path:
                    zip_info = zipfile.ZipInfo(zip_dir_path)
                    zip_info.external_attr = 0o755 << 16
                    zipf.writestr(zip_info, '')
                    empty_dirs_added += 1

        # 再添加所有文件
        print("  添加文件...")
        total_files = 0
        for root, dirs, files in os.walk(MulNX_folder):
            for file in files:
                file_path = Path(root) / file
                arcname = file_path.relative_to(MulNX_parent)
                zipf.write(file_path, arcname)
                total_files += 1
                if total_files % 50 == 0:
                    print(f"    已添加 {total_files} 个文件...")

    zip_size = output_zip_path.stat().st_size
    print(f"  ✓ 压缩完成: {output_zip_path.name} ({zip_size / 1024 / 1024:.2f} MB)")
    print(f"    包含 {total_files} 个文件, {empty_dirs_added} 个空目录")
    return output_zip_path


def main():
    """主函数"""
    print("=" * 60)
    print("MulNX项目打包工具")
    print("=" * 60)

    # 获取脚本所在目录（即 bin 目录）
    script_dir = Path(__file__).parent.resolve()
    print(f"脚本目录: {script_dir}")

    # 检查脚本目录名是否为 'bin'（仅用于提示）
    if script_dir.name.lower() != "bin":
        print(f"警告: 脚本目录不是'bin'目录 (实际: {script_dir.name})")
        in_ci = os.environ.get('CI') == 'true'
        if in_ci:
            print("检测到 CI 环境，自动继续")
        else:
            response = input("是否继续? (y/N): ")
            if response.lower() != 'y':
                print("操作已取消")
                return

    # 定义路径（基于脚本目录）
    output_path = script_dir / "Output"
    MulNX_source = script_dir / "MulNX"
    MulNX_zip_path = script_dir / "MulNX.zip"

    # 0. 检查Output文件夹是否存在
    if not output_path.exists():
        print(f"错误: Output文件夹不存在: {output_path}")
        print("请确保脚本位于bin目录中，且存在Output文件夹")
        return False

    # 1. 清空Output文件夹
    print("\n步骤1: 清空Output文件夹")
    print("-" * 40)
    clear_output_folder(output_path)

    # 2. 复制MulNX文件夹到Output，忽略.pdb文件
    print("\n步骤2: 复制MulNX文件夹到Output")
    print("-" * 40)
    if not MulNX_source.exists():
        print(f"错误: MulNX文件夹不存在: {MulNX_source}")
        return False

    MulNX_output_dir = copy_MulNX_without_pdb(MulNX_source, output_path)

    # 3. 压缩MulNX文件夹为MulNX.zip
    print("\n步骤3: 创建MulNX.zip")
    print("-" * 40)
    if not MulNX_output_dir.exists():
        print(f"错误: MulNX文件夹不存在: {MulNX_output_dir}")
        return False

    zip_result = zip_MulNX_folder(MulNX_output_dir, MulNX_zip_path)

    print("\n" + "=" * 60)
    print("打包完成!")
    print(f"输出位置: {zip_result}")

    # 显示创建的目录结构
    print("\n输出内容结构:")
    print("-" * 40)
    with zipfile.ZipFile(zip_result, 'r') as zipf:
        all_items = zipf.namelist()
        dirs = set()
        files = []
        for item in all_items:
            if item.endswith('/'):
                dirs.add(item)
            else:
                files.append(item)
        print("目录:")
        for d in sorted(dirs):
            print(f"  {d}")
        print("\n文件 (前20个):")
        for f in sorted(files)[:20]:
            print(f"  {f}")
        if len(files) > 20:
            print(f"  ... 和 {len(files) - 20} 个其他文件")

    return True


if __name__ == "__main__":
    try:
        success = main()
        if not success:
            sys.exit(1)
    except KeyboardInterrupt:
        print("\n\n操作被用户中断")
        sys.exit(0)
    except Exception as e:
        print(f"\n错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)