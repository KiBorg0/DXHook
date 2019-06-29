#-------------------------------------------------
#
# Project created by QtCreator 2017-03-17T00:40:08
#
#-------------------------------------------------

QT       += core
QT       -= gui
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -static
#QMAKE_CXXFLAGS += -S main.cpp
#CONFIG += console
TARGET = dx_hook
TEMPLATE = lib
#TEMPLATE = app
CONFIG += dll qt
DEFINES += DXHook_LIBRARY
PROJECT_PATH = "E:/SSStats/release"
DESTDIR      = $$PROJECT_PATH
target.path = $$quote($$(PROGRAMFILES)/Steam/steamapps/common/Dawn of War Soulstorm)
target.files = $$DESTDIR/SSStatsDXHook.dll
INSTALLS += target
INCLUDEPATH += "E:/SSStats/SSStats"
RC_FILE = ssstats.rc
INCLUDEPATH += E:/boost/include/boost-1_63
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
HEADERS += Colors.h \
    cRender.h \
#    Structure.h \
    cMemory.h \
    logger.h \
    hooks.h \
    version.h \
    SetWindowsHookEx-Keylogger.h \
    apmkeyhook.h \
    apmkeyhook_global.h \
    targetver.h
#    ../SSStats/systemwin32.h
SOURCES += main.cpp \
    cRender.cpp \
    cMemory.cpp \
    logger.cpp
#    apmkeyhook.cpp
#    ../SSStats/systemwin32.cpp
unix {
    target.path = /usr/lib
    INSTALLS += target
}
INCLUDEPATH += "C:\\Program Files\\Microsoft DirectX SDK (June 2010)\\Developer Runtime\\x86"
INCLUDEPATH += "C:\\Program Files\\Microsoft DirectX SDK (June 2010)\\Include"

LIBS += -ld3d9
LIBS += -L"C:\\Program Files\\Microsoft DirectX SDK (June 2010)\\Developer Runtime\\x86" -ld3dx9_35
#LIBS += -L"F:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Developer Runtime\\x86" -ld3d9d
#LIBS += -L"F:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Developer Runtime\\x86" -ld3dx9d_33
LIBS += -lgdi32
LIBS += -ldxerr9
LIBS += -lpsapi
#win32:!win32-g++: PRE_TARGETDEPS += $$PWD/'../../Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/d3d9.lib'
#else:win32-g++: PRE_TARGETDEPS += $$PWD/'../../Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/libd3d9.a'

#win32:!win32-g++: PRE_TARGETDEPS += $$PWD/'../../Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/d3dx9.lib'
#else:win32-g++: PRE_TARGETDEPS += $$PWD/'../../Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/libd3dx9.a'

#win32:!win32-g++: PRE_TARGETDEPS += $$PWD/'../../Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/dxerr.lib'
#else:win32-g++: PRE_TARGETDEPS += $$PWD/'../../Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/libdxerr.a'

