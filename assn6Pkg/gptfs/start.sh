sudo rm -rf gptfs_mount/
mkdir gptfs_mount/
sudo PYTHONPATH=$(python -c "import site; print(site.getsitepackages()[0])") python3 src.py /mnt/ramfs