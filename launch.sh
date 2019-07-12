#!/bin/bash

/neventGenerator/zmqGenerator /neventGenerator/rita22012n006190.hdf $ZMQ_HM_PORT >& /tmp/generator &

cd /sinqhm/
./rt/sinqhm_filler zmq_socket=tcp://$HM_HOST:$ZMQ_HM_PORT >& /tmp/sinqhm_filler &
cd ua
./sinqhmegi >& /tmp/sinqhm_server
