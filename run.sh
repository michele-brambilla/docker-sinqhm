#!/bin/bash


/neventGenerator/zmqGenerator /neventGenerator/rita22012n006190.hdf $1 >& /tmp/generator &

cd /sinqhm/
./rt/sinqhm_filler zmq_socket=tcp://localhost:$1 >& /tmp/sinqhm_filler &
cd ua
./sinqhmegi >& /tmp/sinqhm_server
