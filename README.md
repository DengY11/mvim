# mvim

简体中文 | English

---

## 简介（中文）
- 这是一个最小Vim风格的文本编辑器。
- 使用C++20编写，模块化设计，便于扩展。
- 为了编译，你需要安装ncurses库（Linux/FreeBSD/macOS）。

### 性能与大文件
- 读取实现基于 POSIX `mmap`，并结合 `madvise(MADV_SEQUENTIAL)` 做顺序预读，以降低系统调用与缺页开销。
- 在我的 macOS 环境下，针对 100MB 文本的响应时间相较 Vim 更小（结果因平台与配置而异，仅供参考）。

生成测试文件：
```bash
python3 generate_large_file.py 100 large_test_file.txt  # 生成约 100MB 的文本
```

---

## Overview (English)
- This is a minimal Vim-like text editor.
- Written in C++20 with a modular design for easy extension.
- To build, you need the ncurses library (Linux/FreeBSD/macOS).

### Performance & Large Files
- File reading uses POSIX `mmap` plus `madvise(MADV_SEQUENTIAL)` to improve sequential prefetch and reduce syscall/page faults.
- On my macOS machine, opening a 100MB text shows a smaller response time than Vim (results vary across platforms/configs).

Generate a test file:
```bash
python3 generate_large_file.py 100 large_test_file.txt  # generate ~100MB text
```

