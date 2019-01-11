#!/bin/sh

./autogen.sh
./configure --enable-update-desktop-database --enable-doc --enable-dbus --enable-toporouter --enable-nls --with-gui=batch
make
make check
make clean

./autogen.sh
./configure --enable-update-desktop-database --enable-doc --enable-dbus --enable-toporouter --enable-nls --with-gui=gtk
make
#make check
make clean

#./autogen.sh
#./configure --enable-update-desktop-database --enable-doc --enable-dbus --enable-toporouter --enable-nls --with-gui=lesstif
#make
#make check
#make clean
