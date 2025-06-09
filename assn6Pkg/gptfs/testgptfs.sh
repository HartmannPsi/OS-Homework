mkdir gptfs_mount/session_1/
mkdir gptfs_mount/session_2/
echo "Hello ChatGPT" > gptfs_mount/session_1/input
cat gptfs_mount/session_1/output
echo "Hello DeepSeek" > gptfs_mount/session_2/input
cat gptfs_mount/session_2/output
cat gptfs_mount/session_1/output

# Expected Output:
# * Hello ChatGPT
# * Hello DeepSeek
# * Hello ChatGPT