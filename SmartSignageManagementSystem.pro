QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    protocol.cpp \
    qdialogadddev.cpp \
    qdialogsetsign.cpp \
    qdialogsetsigndev.cpp \
    qdlgabout.cpp \
    qdlglogin.cpp \
    qformdebug.cpp \
    qformdebugcmd.cpp \
    qformmain.cpp \
    qformoptions.cpp \
    qformsigntable.cpp \
    signobject.cpp \
    tcpserver.cpp

HEADERS += \
    config.h \
    mainwindow.h \
    protocol.h \
    qdialogadddev.h \
    qdialogsetsign.h \
    qdialogsetsigndev.h \
    qdlgabout.h \
    qdlglogin.h \
    qformdebug.h \
    qformdebugcmd.h \
    qformmain.h \
    qformoptions.h \
    qformsigntable.h \
    signobject.h \
    tcpserver.h

FORMS += \
    mainwindow.ui \
    qdialogadddev.ui \
    qdialogsetsign.ui \
    qdialogsetsigndev.ui \
    qdlgabout.ui \
    qdlglogin.ui \
    qformdebug.ui \
    qformdebugcmd.ui \
    qformmain.ui \
    qformoptions.ui \
    qformsigntable.ui

TRANSLATIONS += \
    SmartSignageManagementSystem_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
