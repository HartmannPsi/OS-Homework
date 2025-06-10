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

    def _new_inode(self, mode):
        inode = self.inode_count
        self.inode_count += 1
        self.inodes[inode] = {
            'mode': mode,
            'nlink': 1,
            'size': 0,
            'ctime': time.time(),
            'mtime': time.time(),
            'atime': time.time(),
        }
        if stat.S_ISDIR(mode):
            self.inodes[inode]['nlink'] = 2
            self.inodes[inode]['children'] = {}
            self.data[inode] = None
        else:
            self.data[inode] = bytearray()
        return inode

    def getattr(self, path, fh=None):
        with self.lock:
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            entry = self.inodes[inode]
            attr = {
                'st_mode': entry['mode'],
                'st_nlink': entry['nlink'],
                'st_size': entry['size'],
                'st_ctime': entry['ctime'],
                'st_mtime': entry['mtime'],
                'st_atime': entry['atime']
            }
            return attr

    def readdir(self, path, fh):
        with self.lock:
            inode = self._get_inode_by_path(path)
            if inode is None:
                raise FuseOSError(errno.ENOENT)
            yield '.'
            yield '..'
            for name in self.inodes[inode].get('children', {}):
                yield name

    def mknod(self, path, mode, dev):
        with self.lock:
            if path in self.path_map:
                raise FuseOSError(errno.EEXIST)
            parent_path, name = os.path.split(path)
            parent_inode = self._get_inode_by_path(parent_path)
            if parent_inode is None:
                raise FuseOSError(errno.ENOENT)
            inode = self._new_inode(mode)
            self.inodes[parent_inode]['children'][name] = inode
            self.path_map[path] = inode

    def mkdir(self, path, mode):
        with self.lock:
            self.mknod(path, stat.S_IFDIR | mode, 0)
            parent_path = os.path.dirname(path)
            parent_inode = self._get_inode_by_path(parent_path)
            self.inodes[parent_inode]['nlink'] += 1

    def unlink(self, path):
        with self.lock:
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

    def rmdir(self, path):
        with self.lock:
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

    def rename(self, old, new):
        with self.lock:
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

    def link(self, target, name):
        with self.lock:
            target_inode = self._get_inode_by_path(target)
            if target_inode is None:
                raise FuseOSError(errno.ENOENT)
            parent_path, fname = os.path.split(name)
            parent_inode = self._get_inode_by_path(parent_path)
            self.inodes[parent_inode]['children'][fname] = target_inode
            self.inodes[target_inode]['nlink'] += 1
            self.path_map[name] = target_inode

    def open(self, path, flags):
        with self.lock:
            if self._get_inode_by_path(path) is None:
                raise FuseOSError(errno.ENOENT)
            self.fd += 1
            return self.fd

    def read(self, path, size, offset, fh):
        with self.lock:
            inode = self._get_inode_by_path(path)
            return self.data[inode][offset:offset + size]

    def write(self, path, data, offset, fh):
        with self.lock:
            inode = self._get_inode_by_path(path)
            buf = self.data[inode]
            new_len = offset + len(data)
            if len(buf) < new_len:
                buf.extend(b'\x00' * (new_len - len(buf)))
            buf[offset:offset + len(data)] = data
            self.inodes[inode]['size'] = len(buf)
            return len(data)

    def truncate(self, path, length, fh=None):
        with self.lock:
            inode = self._get_inode_by_path(path)
            buf = self.data[inode]
            if length < len(buf):
                self.data[inode] = buf[:length]
            else:
                buf.extend(b'\x00' * (length - len(buf)))
            self.inodes[inode]['size'] = length

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print("Usage: python3 ramfs.py <mountpoint>")
        exit(1)
    FUSE(RAMFS(), sys.argv[1], foreground=True, allow_other=True)
