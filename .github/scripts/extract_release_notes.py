import sys
import re

def main():
    if len(sys.argv) < 2:
        print("Usage: extract_release_notes.py <version>")
        sys.exit(1)
        
    version = sys.argv[1]
    
    try:
        with open("release.md", "r") as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Release v{version}")
        sys.exit(0)
        
    # Regex to find:
    # ## [version] or ## version
    # Followed by anything up to the next ## header or end of file
    pattern = rf"(?i)##\s*\[?{re.escape(version)}\]?\s*\n(.*?)(?=\n##\s*|$)"
    match = re.search(pattern, content, re.DOTALL)
    
    if match:
        notes = match.group(1).strip()
        print(notes)
    else:
        print(f"Release v{version}")

if __name__ == "__main__":
    main()
