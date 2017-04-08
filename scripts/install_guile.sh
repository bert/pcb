#!/bin/sh
set -ex
wget https://ftp.gnu.org/gnu/guile/guile-1.8.0.tar.gz -O guile-1.8.0.tar.gz
tar -xzvf guile-1.8.0.tar.gz
cd guile-1.8.0 && ./configure --prefix=/usr && make && sudo make install
