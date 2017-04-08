#!/bin/sh
set -ex
wget https://sourceforge.net/projects/gerbv/files/gerbv/gerbv-2.6.2/gerbv-2.6.2.tar.gz/download -O gerbv-2.6.2.tar.gz
tar -xzvf gerbv-2.6.2.tar.gz
cd gerbv-2.6.2 && ./configure --prefix=/usr && make && sudo make install
