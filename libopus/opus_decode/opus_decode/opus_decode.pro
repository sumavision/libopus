TARGET   = libopus_decode
TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    opus_decode.c

DISTFILES +=

HEADERS += \
    opus_decode.h \
    ../../../opus-1.2.1/include/opus.h \
    ../../../opus-1.2.1/include/opus_custom.h \
    ../../../opus-1.2.1/include/opus_defines.h \
    ../../../opus-1.2.1/include/opus_multistream.h \
    ../../../opus-1.2.1/include/opus_types.h

INCLUDEPATH += \
    $$PWD/../../../opus-1.2.1/include
