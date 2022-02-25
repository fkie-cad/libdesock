#!/usr/bin/env python3

import os

def read_gcov_file(name):
    r = 0
    t = 0
    with open(name) as f:
        for line in f:
            line = line.strip()
            count = line.split(":", 1)[0]
            
            if count != "#####":
                r += 1
            
            t += 1
    return r, t

def main():
    reached = 0
    total = 0
    
    for c_file in os.listdir("../libdesock/src"):
        r, t = read_gcov_file("coverage/" + c_file + ".gcov")
        reached += r
        total += t
        
    print(f"Statement coverage: {100 * reached // total}%")

if __name__ == "__main__":
    main()
