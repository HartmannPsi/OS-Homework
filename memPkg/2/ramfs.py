# ramfs.py
import os
import sys
import errno
import threading
import time
from fuse import FUSE, Operations, FuseOSError

SYNC_DIR = None  # Global sync directory
RAMFS_LOCK = threading.RLock()  # Global lock for thread-safe access

class RamFile:
    def __init__(self):
        self.data = bytearray()
        self.lock = threading.RLock()
        self.atime = time.time()
        self.mtime = self.atime
        self.ctime = self.atime

class Ramfs(Operations):
    def __init__(self):
        self.files = {}
        self.fd_table = {}
        self.fd_count = 0
        self.meta = {}
        self.fd_lock = threading.Lock()

        now = time.time()
        self.meta['/'] = dict(st_mode=(0o40755), st_ctime=now,
                              st_mtime=now, st_atime=now, st_nlink=2)

    def getattr(self, path, fh=None):
        with RAMFS_LOCK:
            if path in self.meta:
                return self.meta[path]
            raise FuseOSError(errno.ENOENT)

    def readdir(self, path, fh):
        with RAMFS_LOCK:
            return ['.', '..'] + [name[1:] for name in self.meta if name != '/' and os.path.dirname(name) == path]

    def create(self, path, mode):
        with RAMFS_LOCK:
            self.meta[path] = dict(st_mode=(0o100644), st_ctime=time.time(),
                                   st_mtime=time.time(), st_atime=time.time(),
                                   st_nlink=1, st_size=0)
            self.files[path] = RamFile()
            self.fd_count += 1
            self.fd_table[self.fd_count] = path
            return self.fd_count

    def open(self, path, flags):
        with RAMFS_LOCK:
            self.fd_count += 1
            self.fd_table[self.fd_count] = path
            return self.fd_count

    def read(self, path, size, offset, fh):
        with RAMFS_LOCK:
            file = self.files[path]
            with file.lock:
                return file.data[offset:offset+size]

    def write(self, path, data, offset, fh):
        with RAMFS_LOCK:
            file = self.files[path]
            with file.lock:
                file.data[offset:offset+len(data)] = data
                self.meta[path]['st_size'] = len(file.data)
                file.mtime = time.time()
            return len(data)

    def truncate(self, path, length, fh=None):
        with RAMFS_LOCK:
            file = self.files[path]
            with file.lock:
                file.data = file.data[:length]
                self.meta[path]['st_size'] = length

    def unlink(self, path):
        with RAMFS_LOCK:
            self.files.pop(path, None)
            self.meta.pop(path, None)

    def flush(self, path, fh):
        return self._flush_file(path)

    def fsync(self, path, fdatasync, fh):
        return self._flush_file(path)

    def _flush_file(self, path):
        if SYNC_DIR is None:
            return 0
        full_path = os.path.join(SYNC_DIR, path.lstrip('/'))
        tmp_path = full_path + '.tmp'
        try:
            os.makedirs(os.path.dirname(full_path), exist_ok=True)
            with open(tmp_path, 'wb') as f:
                f.write(self.files[path].data)
                f.flush()
                os.fsync(f.fileno())
            os.replace(tmp_path, full_path)
        except Exception as e:
            print(f"[ERROR] Flush failed for {path}: {e}", file=sys.stderr)
        return 0

def ramfs_bind(sync_dir):
    global SYNC_DIR
    if not os.path.isdir(sync_dir):
        raise ValueError(f"Sync directory {sync_dir} does not exist")
    SYNC_DIR = os.path.abspath(sync_dir)

def ramfs_file_flush(fp):
    path = os.path.abspath(fp.name).replace('/mnt/ramfs', '')
    if not path.startswith('/'):
        path = '/' + path
    fuse_instance._flush_file(path)  # Access via global FUSE instance

# Global FUSE instance
fuse_instance = Ramfs()

def main(mountpoint, syncdir):
    ramfs_bind(syncdir)
    FUSE(fuse_instance, mountpoint, nothreads=True, foreground=True)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python ramfs.py <mountpoint> <sync_dir>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
