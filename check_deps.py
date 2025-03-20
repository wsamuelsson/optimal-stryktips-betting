#!/usr/bin/env python3
import sys

try:
    import selenium
except ImportError:
    print("Selenium not installed! Please run: pip install -r requirements.txt")
    sys.exit(1)

print("All Python dependencies satisfied.")