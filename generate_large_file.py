#!/usr/bin/env python3
"""
生成大文件用于测试编辑器性能
"""
import os
import random
import string

def generate_large_file(filename, target_size_mb, line_length=80):
    """
    生成指定大小的大文件
    
    Args:
        filename: 输出文件名
        target_size_mb: 目标文件大小（MB）
        line_length: 每行平均长度
    """
    target_size_bytes = target_size_mb * 1024 * 1024
    
    # 生成一些常见的代码模式和单词
    code_patterns = [
        "#include <iostream>",
        "int main() {",
        "for (int i = 0; i < n; i++) {",
        "if (condition) {",
        "while (true) {",
        "return 0;",
        "}",
        "// This is a comment",
        "/* Multi-line",
        "   comment */",
        "std::cout << \"Hello World\" << std::endl;",
        "auto result = function_call(parameter);",
        "class MyClass {",
        "public:",
        "private:",
        "protected:",
        "template<typename T>",
        "namespace my_namespace {",
        "using namespace std;",
        "const int MAX_SIZE = 1000;",
        "static constexpr double PI = 3.14159;",
        "vector<string> data;",
        "map<string, int> counts;",
        "try {",
        "catch (exception& e) {",
        "throw runtime_error(\"error\");",
    ]
    
    common_words = [
        "function", "variable", "constant", "array", "object", "class",
        "method", "property", "iterator", "algorithm", "container", "template",
        "namespace", "exception", "pointer", "reference", "memory", "buffer",
        "string", "integer", "boolean", "character", "floating", "double",
        "public", "private", "protected", "static", "virtual", "const",
        "include", "define", "typedef", "struct", "union", "enum"
    ]
    
    print(f"开始生成 {target_size_mb}MB 的文件: {filename}")
    
    with open(filename, 'w', encoding='utf-8') as f:
        current_size = 0
        line_count = 0
        
        while current_size < target_size_bytes:
            # 随机选择内容类型
            content_type = random.choice(['code', 'text', 'comment', 'blank'])
            
            if content_type == 'code' and random.random() < 0.3:
                # 30%概率插入代码模式
                line = random.choice(code_patterns)
            elif content_type == 'text':
                # 生成随机文本行
                word_count = random.randint(5, 15)
                words = [random.choice(common_words) for _ in range(word_count)]
                line = ' '.join(words) + random.choice([';', ' {', '', ''])
            elif content_type == 'comment':
                # 生成注释
                line = '// ' + ' '.join(random.choice(common_words) for _ in range(random.randint(3, 8)))
            else:
                # 空行或简单行
                line = ''
            
            # 添加行尾
            if line:
                line += '\n'
            else:
                line = '\n'
            
            f.write(line)
            current_size += len(line.encode('utf-8'))
            line_count += 1
            
            # 每10000行打印一次进度
            if line_count % 10000 == 0:
                progress_mb = current_size / (1024 * 1024)
                print(f"进度: {progress_mb:.1f}MB / {target_size_mb}MB (行数: {line_count})")
    
    actual_size_mb = os.path.getsize(filename) / (1024 * 1024)
    print(f"生成完成!")
    print(f"文件名: {filename}")
    print(f"实际大小: {actual_size_mb:.2f} MB")
    print(f"总行数: {line_count:,}")
    print(f"平均每行长度: {current_size/line_count:.1f} 字符")

if __name__ == "__main__":
    import sys
    
    # 默认参数
    filename = "large_test_file.txt"
    size_mb = 100
    
    # 解析命令行参数
    if len(sys.argv) > 1:
        try:
            size_mb = int(sys.argv[1])
        except ValueError:
            print("用法: python generate_large_file.py [大小(MB)] [文件名]")
            sys.exit(1)
    
    if len(sys.argv) > 2:
        filename = sys.argv[2]
    
    generate_large_file(filename, size_mb)