package=qt48
$(package)_version=4.8.7
$(package)_download_path=https://download.qt.io/archive/qt/4.8/$($(package)_version)
$(package)_file_name=qt-everywhere-opensource-src-$($(package)_version).tar.gz
$(package)_sha256_hash=e2882295097e47fe089f8ac741a95fef47e0a73a3f3cdf21b56990638f626ea0
$(package)_dependencies=openssl
$(package)_linux_dependencies=freetype fontconfig dbus libX11 xproto libXext libICE libSM
$(package)_patches= 

define $(package)_set_vars
$(package)_config_opts  = -prefix $(host_prefix) -headerdir $(host_prefix)/include/qt4 -bindir $(build_prefix)/bin
$(package)_config_opts += -release -no-separate-debug-info -opensource -confirm-license
$(package)_config_opts += -stl -qt-zlib

$(package)_config_opts += -nomake examples -nomake tests -nomake tools -nomake translations -nomake demos -nomake docs
$(package)_config_opts += -no-audio-backend -no-glib -no-nis -no-cups -no-iconv -no-gif -no-pch
$(package)_config_opts += -no-xkb -no-xinerama -no-xsync -no-xinput
$(package)_config_opts += -no-libtiff -openssl-linked
$(package)_config_opts += -no-sql-db2 -no-sql-ibase -no-sql-oci -no-sql-tds -no-sql-mysql
$(package)_config_opts += -no-sql-odbc -no-sql-psql -no-sql-sqlite -no-sql-sqlite2
$(package)_config_opts += -no-xmlpatterns -no-multimedia -no-phonon -no-scripttools -no-declarative
$(package)_config_opts += -no-phonon-backend -no-webkit -no-javascript-jit -no-script
$(package)_config_opts += -no-svg -qt-libjpeg -no-libtiff -qt-libpng -no-libmng -no-qt3support -no-opengl

$(package)_config_opts_x86_64_linux  += -platform linux-g++-64 -static -fontconfig -system-freetype
$(package)_config_opts_i686_linux  = -platform linux-g++-32 -static -fontconfig -system-freetype
$(package)_config_opts_mingw32  = -xplatform win32-g++ -platform linux-g++ -no-accessibility -static
$(package)_config_opts_mingw32 += -no-fontconfig -no-freetype -no-dbus -no-glib -no-xkb -no-xrender -no-xrandr
$(package)_config_opts_mingw32 += -no-xfixes -no-xcursor -no-xinerama -no-xsync -no-xinput -no-mitshm -no-xshape
$(package)_config_opts_mingw32 += -no-reduce-exports -no-rpath -force-pkg-config -D QT_NO_TABLETEVENT
$(package)_build_env  = QT_RCC_TEST=1
endef

define $(package)_preprocess_cmds
   sed -i.old "s|/include /usr/include||" config.tests/unix/freetype/freetype.pri && \
   sed -i.old "s|src_plugins.depends = src_gui src_sql src_svg|src_plugins.depends = src_gui src_sql|" src/src.pro && \
   sed -i.old "s|\.lower(|\.toLower(|g" src/network/ssl/qsslsocket_openssl.cpp && \
   sed -i.old "s|Key_BackSpace|Key_Backspace|" src/gui/itemviews/qabstractitemview.cpp && \
   sed -i.old "s|/usr/X11R6/lib64|$(host_prefix)/lib|" mkspecs/*/*.conf && \
   sed -i.old "s|/usr/X11R6/lib|$(host_prefix)/lib|" mkspecs/*/*.conf && \
   sed -i.old "s|/usr/X11R6/include|$(host_prefix)/include|" mkspecs/*/*.conf && \
   sed -i.old "s|QMAKE_LFLAGS_SHLIB\t+= -shared|QMAKE_LFLAGS_SHLIB\t+= -shared -Wl,--exclude-libs,ALL|" mkspecs/common/g++.conf && \
   sed -i.old "/SSLv2_client_method/d" src/network/ssl/qsslsocket_openssl.cpp src/network/ssl/qsslsocket_openssl_symbols.cpp && \
   sed -i.old "/SSLv2_server_method/d" src/network/ssl/qsslsocket_openssl.cpp src/network/ssl/qsslsocket_openssl_symbols.cpp && \
   sed -i.old "/SSLv3_client_method/d" src/network/ssl/qsslsocket_openssl.cpp src/network/ssl/qsslsocket_openssl_symbols.cpp && \
   sed -i.old "/SSLv3_server_method/d" src/network/ssl/qsslsocket_openssl.cpp src/network/ssl/qsslsocket_openssl_symbols.cpp && \
   sed -i.old "s|LIBS += -lbootstrap|LIBS += $($(package)_build_dir)/src/tools/bootstrap/libbootstrap.a|" src/tools/bootstrap/bootstrap.pri && \
   echo "QMAKE_CC = $(host)-gcc" >> mkspecs/win32-g++/qmake.conf && \
   echo "QMAKE_CXX = $(host)-g++" >> mkspecs/win32-g++/qmake.conf && \
   echo "QMAKE_LINK = $(host)-g++" >> mkspecs/win32-g++/qmake.conf && \
   echo "QMAKE_LINK_C = $(host)-gcc" >> mkspecs/win32-g++/qmake.conf && \
   echo "QMAKE_LIB = $(host)-ar -ru" >> mkspecs/win32-g++/qmake.conf && \
   echo "QMAKE_RC = $(host)-windres" >> mkspecs/win32-g++/qmake.conf && \
   sed -i.old "s|typedef QHash<quint64, QTabletDeviceData> QTabletCursorInfo;|#ifndef QT_NO_TABLETEVENT\ntypedef QHash<quint64, QTabletDeviceData> QTabletCursorInfo;|" src/gui/kernel/qapplication_win.cpp && \
   sed -i.old "s|QTabletDeviceData currentTabletPointer;|QTabletDeviceData currentTabletPointer;\n#endif|" src/gui/kernel/qapplication_win.cpp && \
   sed -i.old "s|Q_UNUSED(msg);|#ifndef QT_NO_TABLETEVENT\n    Q_UNUSED(msg);|" src/gui/kernel/qapplication_win.cpp && \
   sed -i.old "s|return sendEvent;|return sendEvent;\n#else\n    return false;\n#endif|" src/gui/kernel/qapplication_win.cpp && \
   sed -i.old "s|win32:SRC_SUBDIRS += src_activeqt|#win32:SRC_SUBDIRS += src_activeqt|" src/src.pro && \
   sed -i.old "s|#include <Windows.h>|#include <windows.h>|" tools/linguist/shared/profileevaluator.cpp && \
   sed -i.old "s|QMAKE_LIBDIR += \$\$QT_BUILD_TREE/src/tools/bootstrap/release|QMAKE_LIBDIR += \$\$QT_BUILD_TREE/src/tools/bootstrap/release \$\$QT_BUILD_TREE/src/tools/bootstrap|" src/tools/bootstrap/bootstrap.pri
endef

define $(package)_config_cmds
  export PKG_CONFIG_SYSROOT_DIR=/ && \
  export PKG_CONFIG_LIBDIR=$(host_prefix)/lib/pkgconfig && \
  export PKG_CONFIG_PATH=$(host_prefix)/share/pkgconfig  && \
  export CPATH=$(host_prefix)/include && \
  OPENSSL_LIBS='-L$(host_prefix)/lib -lssl -lcrypto' ./configure $($(package)_config_opts)
endef

define $(package)_build_cmds
  export CPATH=$(host_prefix)/include && \
  $(MAKE) -C src
endef

define $(package)_stage_cmds
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C src install && \
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  cp bin/qmake $($(package)_staging_prefix_dir)/bin/qmake && \
  cp bin/moc $($(package)_staging_prefix_dir)/bin/moc && \
  cp bin/uic $($(package)_staging_prefix_dir)/bin/uic && \
  cp bin/rcc $($(package)_staging_prefix_dir)/bin/rcc && \
  cp -r plugins $($(package)_staging_prefix_dir)/ && \
  if [ -d imports ]; then cp -r imports $($(package)_staging_prefix_dir)/; fi && \
  cp -r mkspecs $($(package)_staging_prefix_dir)/
endef
