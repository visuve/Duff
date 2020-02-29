QT += core gui widgets

CONFIG += c++17

SOURCES += \
    hashcalculator.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    hashcalculator.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    duff.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:RC_ICONS += duff.ico
