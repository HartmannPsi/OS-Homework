import os
import errno
import time
from fuse import FUSE, Operations, LoggingMixIn

# 模拟 GPT 服务
def simulate_gpt_response(prompt):
    return f"Echo: {prompt.strip()}\n"

class GPTfs(LoggingMixIn, Operations):
    def __init__(self):
        self.files = {}  
        self.fd = 0      

    def _get_session_file(self, path):
        parts = path.strip("/").split("/")
        if len(parts) == 2 and parts[0].startswith("session_"):
            session = f"/{parts[0]}"
            ftype = parts[1]
            return session, ftype
        return None, None

    def getattr(self, path, fh=None):
        now = time.time()
        if path == "/" or path in self.files:
            return dict(st_mode=(0o40755), st_nlink=2, st_ctime=now, st_mtime=now, st_atime=now)
        session, ftype = self._get_session_file(path)
        if session in self.files and ftype in self.files[session]:
            return dict(st_mode=(0o100644), st_nlink=1, st_size=len(self.files[session][ftype]),
                        st_ctime=now, st_mtime=now, st_atime=now)
        raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), path)

    def readdir(self, path, fh):
        if path == "/":
            return ['.', '..'] + [s.strip("/") for s in self.files]
        elif path in self.files:
            return ['.', '..', 'input', 'output', 'error']
        else:
            raise FileNotFoundError

    def mkdir(self, path, mode):
        if path in self.files:
            raise FileExistsError
        self.files[path] = {'input': '', 'output': '', 'error': ''}
        return 0

    def open(self, path, flags):
        self.fd += 1
        return self.fd

    def create(self, path, mode, fi=None):
        return self.open(path, os.O_WRONLY)

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
            self.files[session][ftype] = data.decode()
            return len(data)
        else:
            raise FileNotFoundError

    def read(self, path, size, offset, fh):
        session, ftype = self._get_session_file(path)
        if session and ftype in self.files.get(session, {}):
            content = self.files[session][ftype].encode()
            return content[offset:offset + size]
        raise FileNotFoundError

    def truncate(self, path, length):
        session, ftype = self._get_session_file(path)
        if session and ftype in self.files.get(session, {}):
            self.files[session][ftype] = self.files[session][ftype][:length]
        return 0

if __name__ == '__main__':
    import sys
    import subprocess

    if len(sys.argv) != 2:
        print("Usage: python3 gptfs.py <mountpoint>")
        sys.exit(1)

    mountpoint = sys.argv[1]

    try:
        print(f"Mounting GPTfs at {mountpoint}. Press Ctrl+C to exit.")
        FUSE(GPTfs(), mountpoint, foreground=True, allow_other=True)
    except KeyboardInterrupt:
        print("\nReceived Ctrl+C, attempting to unmount...")
    finally:
        try:
            subprocess.run(["fusermount", "-u", mountpoint], check=True)
            print(f"Unmounted {mountpoint}")
        except subprocess.CalledProcessError:
            print(f"Failed to unmount {mountpoint} — it may already be unmounted or not mounted correctly.")

