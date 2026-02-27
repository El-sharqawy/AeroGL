#!/bin/bash
export MESA_LOADER_DRIVER_OVERRIDE=d3d12
export XDG_SESSION_TYPE=x11
export MESA_NO_ERROR=1
./build/AeroGL
