#!/bin/bash

cd /sinqhm/
./rt/sinqhm_filler >& /tmp/sinqhm_filler &
./nxfiller rita22012n006190.hdf /entry1/RITA-2/detector/counts >& /tmp/filler &
cd ua
./sinqhmegi >& /tmp/sinqhm_server
