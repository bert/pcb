#!/bin/sh
./autogen.sh
./configure --enable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=gtk
make
./configure --enable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=lesstif
make
make distcheck
make -C tests check

