import glob
import os, sys

for file in glob.glob("*.png"):
    os.rename(file, file.split('-')[-1])
