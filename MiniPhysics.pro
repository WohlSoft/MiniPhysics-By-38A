#-------------------------------------------------
#
# Project created by QtCreator 2016-07-17T23:00:32
#
#-------------------------------------------------

QT       += core gui opengl widgets

TARGET = MiniPhysics
TEMPLATE = app

CONFIG += c++11

QMAKE_CXXFLAGS += -ffloat-store

DEFINES += PGE_FILES_QT

SOURCES += main.cpp\
        mainwindow.cpp \
    miniphysics.cpp \
    PGE_File_Formats/file_formats.cpp \
    PGE_File_Formats/file_rw_lvl.cpp \
    PGE_File_Formats/file_rw_lvl_38a.cpp \
    PGE_File_Formats/file_rw_lvlx.cpp \
    PGE_File_Formats/file_rw_meta.cpp \
    PGE_File_Formats/file_rw_npc_txt.cpp \
    PGE_File_Formats/file_rw_sav.cpp \
    PGE_File_Formats/file_rw_savx.cpp \
    PGE_File_Formats/file_rw_smbx64_cnf.cpp \
    PGE_File_Formats/file_rw_wld.cpp \
    PGE_File_Formats/file_rw_wldx.cpp \
    PGE_File_Formats/file_rwopen.cpp \
    PGE_File_Formats/file_strlist.cpp \
    PGE_File_Formats/lvl_filedata.cpp \
    PGE_File_Formats/npc_filedata.cpp \
    PGE_File_Formats/pge_file_lib_globs.cpp \
    PGE_File_Formats/pge_x.cpp \
    PGE_File_Formats/save_filedata.cpp \
    PGE_File_Formats/smbx64.cpp \
    PGE_File_Formats/smbx64_cnf_filedata.cpp \
    PGE_File_Formats/wld_filedata.cpp \
    PGE_File_Formats/ConvertUTF.c

HEADERS  += mainwindow.h \
    miniphysics.h \
    PGE_File_Formats/charsetconvert.h \
    PGE_File_Formats/ConvertUTF.h \
    PGE_File_Formats/CSVReader.h \
    PGE_File_Formats/CSVReaderPGE.h \
    PGE_File_Formats/CSVUtils.h \
    PGE_File_Formats/file_formats.h \
    PGE_File_Formats/file_strlist.h \
    PGE_File_Formats/lvl_filedata.h \
    PGE_File_Formats/meta_filedata.h \
    PGE_File_Formats/npc_filedata.h \
    PGE_File_Formats/pge_file_lib_globs.h \
    PGE_File_Formats/pge_file_lib_sys.h \
    PGE_File_Formats/pge_x.h \
    PGE_File_Formats/pge_x_macro.h \
    PGE_File_Formats/save_filedata.h \
    PGE_File_Formats/smbx64.h \
    PGE_File_Formats/smbx64_cnf_filedata.h \
    PGE_File_Formats/smbx64_macro.h \
    PGE_File_Formats/wld_filedata.h

FORMS    += mainwindow.ui

DISTFILES += \
    PGE_File_Formats/file_rw_lvl_38a.old.txt \
    PGE_File_Formats/LICENSE \
    PGE_File_Formats/Readme.txt \
    PGE_File_Formats/README.md

RESOURCES += \
    res.qrc
