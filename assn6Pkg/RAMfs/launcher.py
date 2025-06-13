import os, sys, subprocess, time

if len(sys.argv) != 2:
    print("Usage: python3 launcher.py <mountpoint>")
    sys.exit(1)

mountpoint = sys.argv[1]

try:
    subprocess.run(["fusermount", "-u", mountpoint], check=False)
    print(f"Launching RAMFS at {mountpoint}. Ctrl+C to unmount.")
    subprocess.run(["python3", "ramfs.py", mountpoint], check=True)
except KeyboardInterrupt:
    print("\n[Ctrl+C] Unmounting...")
    subprocess.run(["fusermount", "-u", mountpoint], check=False)
    time.sleep(0.5)
    os.rmdir(mountpoint)
    print(f"Unmounted and removed {mountpoint}.")
