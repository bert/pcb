#!/bin/sh
set -ex
wget https://ftp.gnu.org/gnu/guile/guile-2.0.0.tar.gz -O guile-2.0.0.tar.gz
tar -xzvf guile-2.0.0.tar.gz
cd guile-2.0.0 && ./configure --prefix=/usr && make && sudo make install
