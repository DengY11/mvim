#!/usr/bin/env python3
"""
Generate a large text file to test editor performance.
"""
import os
import random
import string

def generate_large_file(filename, target_size_mb, line_length=80):
    """
    Generate a large file of the specified size.

    Args:
        filename: output file name
        target_size_mb: target file size in MB
        line_length: average line length (hint, not strict)
    """
    target_size_bytes = target_size_mb * 1024 * 1024
    
    # Common code patterns and words
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
    
    print(f"Start generating {target_size_mb} MB file: {filename}")
    
    with open(filename, 'w', encoding='utf-8') as f:
        current_size = 0
        line_count = 0
        
        while current_size < target_size_bytes:
            # Randomize content type for variety
            content_type = random.choice(['code', 'text', 'comment', 'blank'])

            if content_type == 'code' and random.random() < 0.3:
                # 30% chance to insert a code pattern
                line = random.choice(code_patterns)
            elif content_type == 'text':
                # Generate a random text line
                word_count = random.randint(5, 15)
                words = [random.choice(common_words) for _ in range(word_count)]
                line = ' '.join(words) + random.choice([';', ' {', '', ''])
            elif content_type == 'comment':
                # Generate a comment line
                line = '// ' + ' '.join(random.choice(common_words) for _ in range(random.randint(3, 8)))
            else:
                # Blank line or simple line
                line = ''
            
            # Append newline
            if line:
                line += '\n'
            else:
                line = '\n'
            
            f.write(line)
            current_size += len(line.encode('utf-8'))
            line_count += 1
            
            # Print progress every 10,000 lines
            if line_count % 10000 == 0:
                progress_mb = current_size / (1024 * 1024)
                print(f"Progress: {progress_mb:.1f} MB / {target_size_mb} MB (lines: {line_count})")
    
    actual_size_mb = os.path.getsize(filename) / (1024 * 1024)
    print("Done!")
    print(f"File: {filename}")
    print(f"Actual size: {actual_size_mb:.2f} MB")
    print(f"Total lines: {line_count:,}")
    print(f"Average line length: {current_size/line_count:.1f} chars")

if __name__ == "__main__":
    import sys
    
    # Default parameters
    filename = "large_test_file.txt"
    size_mb = 100
    
    # Parse CLI arguments
    if len(sys.argv) > 1:
        try:
            size_mb = int(sys.argv[1])
        except ValueError:
            print("Usage: python generate_large_file.py [size(MB)] [filename]")
            sys.exit(1)
    
    if len(sys.argv) > 2:
        filename = sys.argv[2]
    
    generate_large_file(filename, size_mb)
