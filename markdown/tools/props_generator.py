# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import os
import re
import sys

def to_pascal_case(kebab_str):
    components = kebab_str.split('-')
    return ''.join(x.title() for x in components)

def to_upper_snake_case(kebab_str):
    return kebab_str.replace('-', '_').upper()

def generate_cpp_enum(props, script_dir):
    # Target header file path
    header_path = os.path.join(script_dir, '../include/markdown/view/markdown_props.h')

    # Generate the new enum content
    new_enum_content = "enum class MarkdownProps {\n"
    for key in props:
        enum_member = "k" + to_pascal_case(key)
        new_enum_content += f"  {enum_member},\n"
    new_enum_content += "};"

    try:
        with open(header_path, 'r') as f:
            header_content = f.read()

        # Regex to find and replace the enum
        pattern = re.compile(r'enum class MarkdownProps\s*\{.*?\};', re.MULTILINE | re.DOTALL)

        if pattern.search(header_content):
            updated_content = pattern.sub(new_enum_content, header_content)

            with open(header_path, 'w') as f:
                f.write(updated_content)
            print(f"Successfully updated {header_path}")
        else:
            print(f"Error: Could not find 'enum class MarkdownProps' in {header_path}")

    except FileNotFoundError:
        print(f"Error: {header_path} not found.")

def generate_java_constants(props, script_dir):
    # Target Java file path
    java_path = os.path.join(script_dir, '../platform/android/serval_markdown/src/main/java/com/lynx/markdown/Constants.java')

    # Generate the new constants content
    new_const_content = ""
    for index, key in enumerate(props):
        const_name = "MARKDOWN_PROPS_" + to_upper_snake_case(key)
        new_const_content += f"  public static final int {const_name} = {index};\n"

    try:
        with open(java_path, 'r') as f:
            java_content = f.read()

        # Regex to find the block of MARKDOWN_PROPS_ constants
        # Matches contiguous lines of public static final int MARKDOWN_PROPS_...
        pattern = re.compile(r'(\s*public static final int MARKDOWN_PROPS_\w+\s*=\s*\d+;\n)+', re.MULTILINE)

        match = pattern.search(java_content)
        if match:
            # Keep the indentation of the first line if possible, or just use the generated content which has 2 spaces
            # The generated content has leading spaces, so we can just replace.
            # However, to be safe with surrounding newlines, let's look at what we matched.
            
            updated_content = java_content[:match.start()] + "\n" + new_const_content + java_content[match.end():]
            
            # Clean up potential extra newlines introduced
            updated_content = re.sub(r'\n{3,}', '\n\n', updated_content)

            with open(java_path, 'w') as f:
                f.write(updated_content)
            print(f"Successfully updated {java_path}")
        else:
            print(f"Error: Could not find 'MARKDOWN_PROPS_' constants block in {java_path}")

    except FileNotFoundError:
        print(f"Error: {java_path} not found.")

def generate_ios_enum(props, script_dir):
    # Target iOS header file path
    ios_header_path = os.path.join(script_dir, '../include/markdown/platform/ios/serval_markdown_props.h')

    # Generate ServalMarkdownProps enum content
    props_enum_body = ""
    for key in props:
        # Use 'Props' (plural) to match the enum name ServalMarkdownProps
        enum_member = "kServalMarkdownProps" + to_pascal_case(key)
        props_enum_body += f"  {enum_member},\n"
    
    props_enum = f"typedef enum : NSUInteger {{\n{props_enum_body}}} ServalMarkdownProps;"

    try:
        with open(ios_header_path, 'r') as f:
            header_content = f.read()

        # Regex to find and replace the ServalMarkdownProps enum
        # Matches "typedef enum : NSUInteger { ... } ServalMarkdownProps;"
        # Uses non-greedy match for content inside braces
        pattern = re.compile(r'typedef enum : NSUInteger\s*\{.*?\}\s*ServalMarkdownProps;', re.MULTILINE | re.DOTALL)

        if pattern.search(header_content):
            updated_content = pattern.sub(props_enum, header_content)

            with open(ios_header_path, 'w') as f:
                f.write(updated_content)
            print(f"Successfully updated {ios_header_path}")
        else:
            print(f"Error: Could not find 'ServalMarkdownProps' enum in {ios_header_path}")

    except FileNotFoundError:
        print(f"Error: {ios_header_path} not found.")

def main():
    # Determine the directory of the script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    json_path = os.path.join(script_dir, 'markdown_props.json')

    try:
        with open(json_path, 'r') as f:
            props = json.load(f)
    except FileNotFoundError:
        print(f"Error: {json_path} not found.")
        return

    generate_cpp_enum(props, script_dir)
    generate_java_constants(props, script_dir)
    generate_ios_enum(props, script_dir)

if __name__ == "__main__":
    main()
