TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

include($$PWD/../../$$DWYCO_CONFDIR/conf.pri)

INCLUDEPATH += 
macx-g++|linux-g++|linux-g++-64 {
DEFINES += HAVE_UNISTD_H 
#QMAKE_CXXFLAGS += -fpermissive
#QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-reorder
}

SOURCES = \
adler32.c compress.c crc32.c uncompr.c deflate.c trees.c \
zutil.c inflate.c infblock.c inftrees.c infcodes.c infutil.c inffast.c
