import sys
import os
from ramfs import RAMfs
from fuse import FUSE

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 launcher.py <mountpoint> <persist_dir>")
        exit(1)

    mountpoint = sys.argv[1]
    persist_dir = sys.argv[2]
    os.makedirs(persist_dir, exist_ok=True)
    FUSE(RAMfs(persist_dir), mountpoint, nothreads=False, foreground=True, allow_other=True, rw=True)
