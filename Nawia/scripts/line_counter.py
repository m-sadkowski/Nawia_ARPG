import os  
import subprocess

def count_lines_of_code():

  result = subprocess.run(['git', 'ls-files'], capture_output=True, text=True)
  files = result.stdout.splitlines()

  total_lines = 0
  for file in files:
    with open(file, 'r', errors='ignore') as f:
      total_lines += sum(1 for _ in f)

  print(f"Total lines of code: {total_lines}")

if __name__ == "__main__":
  count_lines_of_code()