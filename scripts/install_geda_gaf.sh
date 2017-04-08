#!/bin/sh
set -ex
wget http://ftp.geda-project.org/geda-gaf/stable/v1.8/1.8.2/geda-gaf-1.8.2.tar.gz
tar -xzvf geda-gaf-1.8.2.tar.gz
cd geda-gaf-1.8.2 && ./configure --prefix=/usr && make && sudo make install
