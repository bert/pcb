#!/bin/sh
./autogen.sh
./configure --enable-update-desktop-database --enable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=gtk
make
make check
make distclean
./configure --enable-update-desktop-database --enable-doc --enable-dbus --disable-toporouter --enable-nls --with-gui=lesstif
make
make check

