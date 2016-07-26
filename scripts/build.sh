#!/bin/sh
./autogen.sh
./configure --disable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=gtk
make
./configure --disable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=lesstif
make
make distcheck
make -C tests check

