# 使用 FUSE 实现一个模拟 GPT 文件系统

import os
import errno
import time
from fuse import FUSE, Operations, LoggingMixIn

# 模拟 GPT 服务（可改为调用 OpenAI API）
def simulate_gpt_response(prompt):
    return f"Echo: {prompt.strip()}\n"

class GPTfs(LoggingMixIn, Operations):
    def __init__(self):
        self.files = {}  # 保存 session -> {input, output, error}
        self.fd = 0      # 文件描述符模拟值

    # 从路径中解析出 session 名称和文件类型，例如：/session_1/input -> ('/session_1', 'input')
    def _get_session_file(self, path):
        parts = path.strip("/").split("/")
        if len(parts) == 2 and parts[0].startswith("session_"):
            session = f"/{parts[0]}"
            ftype = parts[1]
            return session, ftype
        return None, None

    # 返回文件或目录的属性信息
    def getattr(self, path, fh=None):
        now = time.time()
        if path == "/" or path in self.files:
            return dict(st_mode=(0o40755), st_nlink=2, st_ctime=now, st_mtime=now, st_atime=now)
        session, ftype = self._get_session_file(path)
        if session in self.files and ftype in self.files[session]:
            return dict(st_mode=(0o100644), st_nlink=1, st_size=len(self.files[session][ftype]),
                        st_ctime=now, st_mtime=now, st_atime=now)
        raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), path)

    # 列出目录下的文件
    def readdir(self, path, fh):
        if path == "/":
            return ['.', '..'] + [s.strip("/") for s in self.files]
        elif path in self.files:
            return ['.', '..', 'input', 'output', 'error']
        else:
            raise FileNotFoundError

    # 创建新的会话目录
    def mkdir(self, path, mode):
        if path in self.files:
            raise FileExistsError
        self.files[path] = {'input': '', 'output': '', 'error': ''}
        return 0

    # 打开文件时分配一个伪文件描述符
    def open(self, path, flags):
        self.fd += 1
        return self.fd

    # 创建文件时分配文件描述符，通常不需要特别处理，因为文件在写入时就已存在
    def create(self, path, mode, fi=None):
        return self.open(path, os.O_WRONLY)

    # 对 input 文件的写入操作会触发 GPT 模拟处理
    def write(self, path, data, offset, fh):
        session, ftype = self._get_session_file(path)
        if session and ftype == "input":
            self.files[session][ftype] = data.decode()
            try:
                result = simulate_gpt_response(self.files[session][ftype])
                self.files[session]["output"] = result
                self.files[session]["error"] = ""
            except Exception as e:
                self.files[session]["output"] = ""
                self.files[session]["error"] = str(e)
            return len(data)
        elif session and ftype in ["output", "error"]:
            # 通常不会对这些文件写入，但可允许
            self.files[session][ftype] = data.decode()
            return len(data)
        else:
            raise FileNotFoundError

    # 读取文件内容
    def read(self, path, size, offset, fh):
        session, ftype = self._get_session_file(path)
        if session and ftype in self.files.get(session, {}):
            content = self.files[session][ftype].encode()
            return content[offset:offset + size]
        raise FileNotFoundError

    # 清空文件
    def truncate(self, path, length):
        session, ftype = self._get_session_file(path)
        if session and ftype in self.files.get(session, {}):
            self.files[session][ftype] = self.files[session][ftype][:length]
        return 0

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print("Usage: python3 gptfs.py <mountpoint>")
        sys.exit(1)

    mountpoint = sys.argv[1]
    FUSE(GPTfs(), mountpoint, foreground=True, allow_other=True)
