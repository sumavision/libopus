TEMPLATE = subdirs
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c

SUBDIRS += \
    opus_encode/opus_encode \
    opus_demo/opus_demo \
    opus_decode/opus_decode
