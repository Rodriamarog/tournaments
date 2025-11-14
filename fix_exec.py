#!/usr/bin/env python3
import re
import sys

def fix_exec_params(content):
    # Pattern to match tx.exec( lines followed by parameters
    # This handles both single-line and multi-line exec calls

    # First, handle simple single-parameter cases
    content = re.sub(
        r'tx\.exec\(\s*"([^"]+)",\s*([^)]+)\s*\)',
        r'tx.exec("\1", pqxx::params(\2))',
        content
    )

    return content

if __name__ == "__main__":
    filepath = sys.argv[1]

    with open(filepath, 'r') as f:
        content = f.read()

    fixed_content = fix_exec_params(content)

    with open(filepath, 'w') as f:
        f.write(fixed_content)

    print(f"Fixed {filepath}")
