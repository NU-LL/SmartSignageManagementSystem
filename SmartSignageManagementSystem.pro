QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    ui/mainwindow.cpp \
    protocol/protocol.cpp \
    ui/qdialogadddev.cpp \
    ui/qdialogsetsign.cpp \
    ui/qdialogsetsigndev.cpp \
    ui/qdlgabout.cpp \
    ui/qdlglogin.cpp \
    ui/qformdebug.cpp \
    ui/qformdebugcmd.cpp \
    ui/qformmain.cpp \
    ui/qformoptions.cpp \
    ui/qformsigntable.cpp \
    device/tcpserver.cpp \
    device/tcpsigndevice.cpp

HEADERS += \
    config.h \
    ui/mainwindow.h \
    protocol/protocol.h \
    ui/qdialogadddev.h \
    ui/qdialogsetsign.h \
    ui/qdialogsetsigndev.h \
    ui/qdlgabout.h \
    ui/qdlglogin.h \
    ui/qformdebug.h \
    ui/qformdebugcmd.h \
    ui/qformmain.h \
    ui/qformoptions.h \
    ui/qformsigntable.h \
    device/tcpserver.h \
    device/tcpsigndevice.h

FORMS += \
    ui/qt_ui_file/mainwindow.ui \
    ui/qt_ui_file/qdialogadddev.ui \
    ui/qt_ui_file/qdialogsetsign.ui \
    ui/qt_ui_file/qdialogsetsigndev.ui \
    ui/qt_ui_file/qdlgabout.ui \
    ui/qt_ui_file/qdlglogin.ui \
    ui/qt_ui_file/qformdebug.ui \
    ui/qt_ui_file/qformdebugcmd.ui \
    ui/qt_ui_file/qformmain.ui \
    ui/qt_ui_file/qformoptions.ui \
    ui/qt_ui_file/qformsigntable.ui

TRANSLATIONS += \
    SmartSignageManagementSystem_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icon.qrc
