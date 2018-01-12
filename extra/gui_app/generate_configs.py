#!/usr/bin/env python

import json 
import sys
from scipy import signal

filename = sys.argv[1]
dst_path = sys.argv[2]
orig = json.load(open(filename))

counter = 0
html_lines = []
TF_SEGMENT_SIZE = 16 

"""
#opacity attenuation
for i in range(0, 100, 5):
    i = i / 100.0
    orig["opacityAttenuation"] = i
    new_filename = "temp" + str(counter) + ".json"
    with open(dst_path + '/' + new_filename, 'w') as fp:
        json.dump(orig, fp)
    html_lines.append("<img class='hyperimage' data-dataset='temp" + str(counter) + "' width='150' height='150'/>")
    counter += 1
"""

def get_tf_seed(n, size, val): # Creates a list of size `size` with the first n elemenst as val and the rest as 0
    seq  = [val] * n + [0] * (size - n)
    return seq

def rshift(seq):
    seq.pop()
    seq = [0] + seq
    return seq

def get_tfs(size):
    vals = [1, 16, 64, 128]
    ns = range(1, size + 1)
    tfs = []
    win = signal.hann(20)
    for val in vals:
        for n in ns:
            seed = get_tf_seed(n, size, val)
            tfs.append(seed[:])
            n_shifts = size - n
            for _ in range(n_shifts):
                seed = rshift(seed)
                tfs.append(seed[:])
                # convolve with Hann
                filtered = signal.convolve(seed, win, mode='same') / sum(win)
                tfs.append(filtered[:].tolist())

    tfs.append([0] * size)
    return tfs

tfs = get_tfs(TF_SEGMENT_SIZE)
tfs_len = len(tfs)

for i in tfs:
    opacity_map = [x / 256.0 for x in i]
    orig["opacityMap"] = opacity_map
    new_filename = "temp" + str(counter) + ".json"
    with open(dst_path + '/' + new_filename, 'w') as fp:
        json.dump(orig, fp)
    html_lines.append("<img class='hyperimage' data-dataset='temp" + str(counter) + "' width='150' height='150'/>")
    counter += 1

html = open("app.html.template").read()
html_lines = "\n".join(html_lines)
html = html.replace("<%DESIGN_GALLERY%>", html_lines)
with open("app.html", 'w') as fp:
    fp.write(html)

