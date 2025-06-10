import os, time, errno, threading
from collections import defaultdict
from fuse import FUSE, Operations, LoggingMixIn, FuseOSError
import stat

class RAMFS(LoggingMixIn, Operations):
    def __init__(self):
        self.lock = threading.RLock()
        self.inodes = {}
        self.data = {}
        self.path_map = {'/': 1}
        self.inodes[1] = {
            'mode': stat.S_IFDIR | 0o755,
            'nlink': 2,
            'size': 0,
            'ctime': time.time(),
            'mtime': time.time(),
            'atime': time.time(),
            'children': {}
        }
        self.data[1] = None
        self.fd = 0
        self.inode_count = 2  # next inode id

def _get_inode_by_path(self, path):
    return self.path_map.get(path)

def _log_error(self, name, path, error):
    print(f"[{name}][ERROR] path={path}: {error}")

def getattr(self, path, fh=None):
    with self.lock:
        try:
            print(f"[getattr] {path}")
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            entry = self.inodes[inode]
            return {
                'st_mode': entry['mode'],
                'st_nlink': entry['nlink'],
                'st_size': entry['size'],
                'st_ctime': entry['ctime'],
                'st_mtime': entry['mtime'],
                'st_atime': entry['atime']
            }
        except Exception as e:
            self._log_error("getattr", path, e)
            raise FuseOSError(errno.EIO)

def readdir(self, path, fh):
    with self.lock:
        try:
            print(f"[readdir] {path}")
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            yield '.'
            yield '..'
            for name in self.inodes[inode]['children']:
                yield name
        except Exception as e:
            self._log_error("readdir", path, e)
            raise FuseOSError(errno.EIO)

def mknod(self, path, mode, dev):
    with self.lock:
        try:
            print(f"[mknod] {path}")
            if path in self.path_map:
                raise FuseOSError(errno.EEXIST)
            parent_path, name = os.path.split(path)
            parent_inode = self._get_inode_by_path(parent_path)
            if parent_inode is None:
                raise FuseOSError(errno.ENOENT)
            inode = self.inode_count
            self.inode_count += 1
            self.inodes[inode] = {
                'mode': mode,
                'nlink': 1,
                'size': 0,
                'ctime': time.time(),
                'mtime': time.time(),
                'atime': time.time()
            }
            self.data[inode] = bytearray()
            self.inodes[parent_inode]['children'][name] = inode
            self.path_map[path] = inode
        except Exception as e:
            self._log_error("mknod", path, e)
            raise FuseOSError(errno.EIO)

def create(self, path, mode, fi=None):
    with self.lock:
        try:
            print(f"[create] {path}")
            self.mknod(path, mode, 0)
            self.fd += 1
            return self.fd
        except Exception as e:
            self._log_error("create", path, e)
            raise FuseOSError(errno.EIO)

def mkdir(self, path, mode):
    with self.lock:
        try:
            print(f"[mkdir] {path}")
            self.mknod(path, stat.S_IFDIR | mode, 0)
            parent_path = os.path.dirname(path)
            parent_inode = self._get_inode_by_path(parent_path)
            self.inodes[parent_inode]['nlink'] += 1
            self.data[self.path_map[path]] = None
            self.inodes[self.path_map[path]]['children'] = {}
        except Exception as e:
            self._log_error("mkdir", path, e)
            raise FuseOSError(errno.EIO)

def unlink(self, path):
    with self.lock:
        try:
            print(f"[unlink] {path}")
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            parent_path, name = os.path.split(path)
            parent_inode = self._get_inode_by_path(parent_path)
            del self.inodes[parent_inode]['children'][name]
            self.inodes[inode]['nlink'] -= 1
            if self.inodes[inode]['nlink'] == 0:
                del self.inodes[inode]
                del self.data[inode]
            del self.path_map[path]
        except Exception as e:
            self._log_error("unlink", path, e)
            raise FuseOSError(errno.EIO)

def rmdir(self, path):
    with self.lock:
        try:
            print(f"[rmdir] {path}")
            inode = self._get_inode_by_path(path)
            if inode is None or not stat.S_ISDIR(self.inodes[inode]['mode']):
                raise FuseOSError(errno.ENOTDIR)
            if self.inodes[inode]['children']:
                raise FuseOSError(errno.ENOTEMPTY)
            parent_path = os.path.dirname(path)
            parent_inode = self._get_inode_by_path(parent_path)
            name = os.path.basename(path)
            del self.inodes[parent_inode]['children'][name]
            self.inodes[parent_inode]['nlink'] -= 1
            del self.inodes[inode]
            del self.data[inode]
            del self.path_map[path]
        except Exception as e:
            self._log_error("rmdir", path, e)
            raise FuseOSError(errno.EIO)

def rename(self, old, new):
    with self.lock:
        try:
            print(f"[rename] {old} -> {new}")
            old_inode = self._get_inode_by_path(old)
            if old_inode is None:
                raise FuseOSError(errno.ENOENT)
            old_parent = os.path.dirname(old)
            new_parent = os.path.dirname(new)
            old_name = os.path.basename(old)
            new_name = os.path.basename(new)
            old_parent_inode = self._get_inode_by_path(old_parent)
            new_parent_inode = self._get_inode_by_path(new_parent)
            self.inodes[new_parent_inode]['children'][new_name] = old_inode
            del self.inodes[old_parent_inode]['children'][old_name]
            del self.path_map[old]
            self.path_map[new] = old_inode
        except Exception as e:
            self._log_error("rename", old, e)
            raise FuseOSError(errno.EIO)

def link(self, target, name):
    with self.lock:
        try:
            print(f"[link] target: {target}, name: {name}")
            target_inode = self._get_inode_by_path(target)
            if target_inode is None:
                raise FuseOSError(errno.ENOENT)
            parent_path, fname = os.path.split(name)
            parent_inode = self._get_inode_by_path(parent_path)
            self.inodes[parent_inode]['children'][fname] = target_inode
            self.inodes[target_inode]['nlink'] += 1
            self.path_map[name] = target_inode
        except Exception as e:
            self._log_error("link", name, e)
            raise FuseOSError(errno.EIO)

def open(self, path, flags):
    with self.lock:
        try:
            print(f"[open] {path}")
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            self.fd += 1
            return self.fd
        except Exception as e:
            self._log_error("open", path, e)
            raise FuseOSError(errno.EIO)

def read(self, path, size, offset, fh):
    with self.lock:
        try:
            print(f"[read] {path}, offset={offset}, size={size}")
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            return self.data[inode][offset:offset + size]
        except Exception as e:
            self._log_error("read", path, e)
            raise FuseOSError(errno.EIO)

def write(self, path, data, offset, fh):
    with self.lock:
        try:
            print(f"[write] {path}, offset={offset}, len(data)={len(data)}")
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            buf = self.data[inode]
            new_len = offset + len(data)
            if len(buf) < new_len:
                buf.extend(b'\x00' * (new_len - len(buf)))
            buf[offset:offset + len(data)] = data
            self.inodes[inode]['size'] = len(buf)
            return len(data)
        except Exception as e:
            self._log_error("write", path, e)
            raise FuseOSError(errno.EIO)

def truncate(self, path, length, fh=None):
    with self.lock:
        try:
            print(f"[truncate] {path}, length={length}")
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            buf = self.data[inode]
            if length < len(buf):
                self.data[inode] = buf[:length]
            else:
                buf.extend(b'\x00' * (length - len(buf)))
            self.inodes[inode]['size'] = length
        except Exception as e:
            self._log_error("truncate", path, e)
            raise FuseOSError(errno.EIO)

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print("Usage: python3 ramfs.py <mountpoint>")
        exit(1)
    FUSE(RAMFS(), sys.argv[1], foreground=True, allow_other=True)
