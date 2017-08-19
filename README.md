# Enchiladas

A lightweight web-based viewer for OSPRay. 

Currently depends on:

- PBNJ
- Pistache (https://github.com/alanbirtles/pistache)

## Setup

- `git clone` this repo.
- Make a `resources` directory and clone pbnj (https://github.com/seelabutk/pbnj).
- Also clone the Pistache repo from Alan Birtles (https://github.com/alanbirtles/pistache) in `resources`. Specifically, we've tested revision 9d5a4132a58e4685b2a01da1496447c2e8625717.
- Make a build directory in the root and run `cmake ../` inside it. 
- Run ccmake . and set the ospray build directory (e.g. ~/ospray/1.1.0/build).
- Configure and make.
