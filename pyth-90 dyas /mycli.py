import argparse
import subprocess

parser = argparse.ArgumentParser(
    description="My CLI tool"
)
parser.add_argument("url", help = "Video CLI tool")
parser.add_argument("-f", "--format", default="mp4", help="output format")

args = parser.parse_args()

subprocess.run(["yt-dlp", args.url])
print(f"Downlading from: {args.url}")
print(f"Format: {args.format}")