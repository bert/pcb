#!/bin/sh
set -ex
wget https://ftp.gnu.org/gnu/gettext/gettext-0.19.3.tar.gz -O gettext-0.19.3.tar.gz
tar -xzvf gettext-0.19.3.tar.gz
cd gettext-0.19.3 && ./configure --prefix=/usr && make && sudo make install
