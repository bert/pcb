#!/bin/sh
./autogen.sh
./configure --enable-update-desktop-database --enable-doc --enable-dbus --enable-toporouter --enable-nls --with-gui=gtk
make
make check
