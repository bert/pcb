#!/bin/sh
set -ex
wget ftp://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz -O autoconf-2.69.tar.gz
tar -xzvf autoconf-2.69.tar.gz
cd autoconf-2.69 && ./configure --prefix=/usr && make && sudo make install
