# mvim
<img width="743" height="815" alt="image" src="https://github.com/user-attachments/assets/157e0601-7afe-4adf-a6db-cf7c7ba0bae9" />

简体中文 | English

---

## 简介（中文）
- 这是一个最小Vim风格的文本编辑器。
- 使用C++20编写，模块化设计，便于扩展。
- 为了编译，你需要安装ncurses库（Linux/FreeBSD/macOS）。

### 性能与大文件
- 读取实现基于 POSIX `mmap`，并结合 `madvise(MADV_SEQUENTIAL)` 做顺序预读，以降低系统调用与缺页开销。
- 编辑核心提供了rope tree\vector\gap buffer三种后端，你可以在`config.hpp`中选择。目前rope tree在大部分场景性能最好，一些场景不如vector, gap buffer已经废弃了，不建议使用。vector经过我的广泛测试，rope tree还没有经过广泛验证

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
- The editor core provides three backends: rope tree, vector, and gap buffer. You can choose the backend in `config.hpp`. Currently, rope tree performs best in most scenarios, while vector has been extensively tested. Gap buffer is deprecated and not recommended.

Generate a test file:
```bash
python3 generate_large_file.py 100 large_test_file.txt  # generate ~100MB text
```

