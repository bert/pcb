#!/bin/sh
./autogen.sh
./configure --disable-update-desktop-database --disable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=gtk
make
./configure --disable-update-desktop-database --disable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=lesstif
make
make distcheck
make -C tests check

