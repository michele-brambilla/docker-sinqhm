#!/bin/bash

cd /sinqhm/
./rt/sinqhm_filler zmq_socket=tcp://generator:5557 &
cd ua
./sinqhmegi
