QT       += core gui serialbus serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common_tools/common_tool_func.cpp \
    hv_connsettings.cpp \
    hv_tester/hvtester.cpp \
    logger/logger.cpp \
    modbus_regs/modbus_regs.cpp \
    test_param_settings.cpp \
    main.cpp \
    main_dialog.cpp \
    version_def/version_def.cpp

HEADERS += \
    common_tools/common_tool_func.h \
    hv_connsettings.h \
    hv_tester/hvtester.h \
    logger/logger.h \
    modbus_regs/modbus_regs.h \
    test_param_settings.h \
    main_dialog.h \
    test_params_struct.h \
    version_def/version_def.h

FORMS += \
    hv_connsettings.ui \
    main_dialog.ui \
    test_param_settings.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
msvc:QMAKE_CXXFLAGS += -execution-charset:utf-8
