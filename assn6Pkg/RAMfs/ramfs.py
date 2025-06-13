import os
import sys
import errno
import time
import threading
import shutil
from collections import defaultdict
from fuse import FUSE, FuseOSError, Operations, LoggingMixIn
from stat import S_IFDIR, S_IFREG
from datetime import datetime
from pathlib import Path
import logging

BLOCK_SIZE = 4096

class Inode:
    def __init__(self, mode, is_dir=False):
        self.mode = mode
        self.atime = self.mtime = self.ctime = time.time()
        self.size = 0
        self.nlink = 1
        self.data = bytearray()
        self.is_dir = is_dir
        self.children = {} if is_dir else None
        self.lock = threading.RLock()

class RAMfs(LoggingMixIn, Operations):
    def __init__(self, persist_dir):
        self.root = Inode(S_IFDIR | 0o755, is_dir=True)
        self.fd = 0
        self.inodes = {'/': self.root}
        self.paths = {'/': self.root}
        self.persist_dir = persist_dir
        self.global_lock = threading.RLock()
        logging.basicConfig(level=logging.DEBUG)

    def _full_path(self, path):
        return os.path.join(self.persist_dir, path.lstrip('/'))

    def getattr(self, path, fh=None):
        inode = self.paths.get(path)
        if not inode:
            raise FuseOSError(errno.ENOENT)
        st = {
            'st_mode': inode.mode,
            'st_nlink': inode.nlink,
            'st_size': inode.size,
            'st_ctime': inode.ctime,
            'st_mtime': inode.mtime,
            'st_atime': inode.atime,
        }
        return st

    def readdir(self, path, fh):
        inode = self.paths.get(path)
        if not inode or not inode.is_dir:
            raise FuseOSError(errno.ENOTDIR)
        with inode.lock:
            return ['.', '..'] + list(inode.children.keys())

    def mkdir(self, path, mode):
        with self.global_lock:
            parent = self._get_parent(path)
            name = os.path.basename(path)
            if name in parent.children:
                raise FuseOSError(errno.EEXIST)
            node = Inode(S_IFDIR | mode, is_dir=True)
            parent.children[name] = node
            self.paths[path] = node
            return 0

    def mknod(self, path, mode, dev):
        with self.global_lock:
            parent = self._get_parent(path)
            name = os.path.basename(path)
            if name in parent.children:
                raise FuseOSError(errno.EEXIST)
            node = Inode(S_IFREG | mode)
            parent.children[name] = node
            self.paths[path] = node
            return 0
        
    def create(self, path, mode, fi=None):
        return self.mknod(path, mode, 0)

    def unlink(self, path):
        with self.global_lock:
            parent = self._get_parent(path)
            name = os.path.basename(path)
            if name not in parent.children:
                raise FuseOSError(errno.ENOENT)
            node = parent.children[name]
            node.nlink -= 1
            if node.nlink == 0:
                del self.paths[path]
            del parent.children[name]

    def rmdir(self, path):
        inode = self.paths.get(path)
        if not inode or not inode.is_dir:
            raise FuseOSError(errno.ENOTDIR)
        if inode.children:
            raise FuseOSError(errno.ENOTEMPTY)
        parent = self._get_parent(path)
        name = os.path.basename(path)
        del parent.children[name]
        del self.paths[path]

    def rename(self, old, new):
        with self.global_lock:
            old_parent = self._get_parent(old)
            new_parent = self._get_parent(new)
            name_old = os.path.basename(old)
            name_new = os.path.basename(new)
            node = old_parent.children.pop(name_old)
            new_parent.children[name_new] = node
            del self.paths[old]
            self.paths[new] = node

    def link(self, target, name):
        with self.global_lock:
            inode = self.paths.get(target)
            if not inode:
                raise FuseOSError(errno.ENOENT)
            parent = self._get_parent(name)
            name_final = os.path.basename(name)
            parent.children[name_final] = inode
            inode.nlink += 1
            self.paths[name] = inode

    def open(self, path, flags):
        inode = self.paths.get(path)
        if not inode:
            raise FuseOSError(errno.ENOENT)
        self.fd += 1
        return self.fd

    def read(self, path, size, offset, fh):
        inode = self.paths.get(path)
        if not inode:
            raise FuseOSError(errno.ENOENT)
        with inode.lock:
            return inode.data[offset:offset + size]

    def write(self, path, data, offset, fh):
        inode = self.paths.get(path)
        if not inode:
            raise FuseOSError(errno.ENOENT)
        with inode.lock:
            inode.size = max(inode.size, offset + len(data))
            if len(inode.data) < offset + len(data):
                inode.data.extend(b'\x00' * (offset + len(data) - len(inode.data)))
            inode.data[offset:offset + len(data)] = data
            inode.mtime = time.time()
            return len(data)
        
    def truncate(self, path, length, fh=None):
        with self.global_lock:
            if path not in self.paths:
                raise FuseOSError(errno.ENOENT)
            inode = self.paths[path]
            with inode.lock:
                current_size = inode.size
                if length < current_size:
                    inode.data = inode.data[:length]
                else:
                    inode.data += bytearray(length - current_size)
                inode.size = length
                inode.mtime = inode.ctime = time.time()

    def chmod(self, path, mode):
        return 0

    def chown(self, path, uid, gid):
        return 0

    def utimens(self, path, times=None):
        now = time.time()
        atime, mtime = times if times else (now, now)
        with self.global_lock:
            if path not in self.paths:
                raise FuseOSError(errno.ENOENT)
            inode = self.paths[path]
            with inode.lock:
                inode.atime = atime
                inode.mtime = mtime

    def flush(self, path, fh):
        inode = self.paths.get(path)
        if inode.is_dir:
            return 0

        full_path = self._full_path(path)
        temp_path = full_path + ".tmp"

        try:
            # 确保目录存在
            os.makedirs(os.path.dirname(full_path), exist_ok=True)

            # 写入临时文件
            with open(temp_path, 'wb') as f:
                with inode.lock:
                    f.write(inode.data)

            # 原子替换
            os.replace(temp_path, full_path)
            return 0

        except PermissionError as e:
            print(f"[flush] Permission denied when writing to {full_path}")
            raise FuseOSError(errno.EPERM)
        except FileNotFoundError as e:
            print(f"[flush] Persist directory missing: {self.persist_dir}")
            raise FuseOSError(errno.ENOENT)
        except Exception as e:
            print(f"[flush] Unexpected error during flush for {path}: {e}")
            raise FuseOSError(errno.EIO)
            
    def _get_parent(self, path):
        parent_path = os.path.dirname(path) or '/'
        return self.paths.get(parent_path)

