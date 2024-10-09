QT       += core gui serialbus serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

#INCLUDEPATH += "D:/01.Prog/Qt5.15/5.15.2/msvc2019_64/include/QtSerialBus/5.15.2/QtSerialBus" \
#"D:/01.Prog/Qt5.15/5.15.2/msvc2019_64/include/QtCore/5.15.2/QtCore" \
#"D:/01.Prog/Qt5.15/5.15.2/msvc2019_64/include/QtCore/5.15.2"

INCLUDEPATH += "$$[QT_INSTALL_HEADERS]/QtSerialBus/$$[QT_VERSION]/QtSerialBus" \
"$$[QT_INSTALL_HEADERS]/QtCore/$$[QT_VERSION]/QtCore" \
"$$[QT_INSTALL_HEADERS]/QtCore/$$[QT_VERSION]"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common_tools/common_tool_func.cpp \
    config_recorder/uiconfigrecorder.cpp \
    hv_connsettings.cpp \
    hv_tester/hvtester.cpp \
    logger/logger.cpp \
    mb_rtu_over_tcp/qmodbusrtuovertcpclient.cpp \
    modbus_regs/modbus_regs.cpp \
    sysconfigs/sysconfigs.cpp \
    test_param_settings.cpp \
    main.cpp \
    main_dialog.cpp \
    test_result_judge/test_result_judge.cpp \
    version_def/version_def.cpp

HEADERS += \
    common_tools/common_tool_func.h \
    config_recorder/uiconfigrecorder.h \
    hv_connsettings.h \
    hv_tester/hvtester.h \
    logger/logger.h \
    mb_rtu_over_tcp/private/exposed_qmodbusadu_p.h \
    mb_rtu_over_tcp/private/qmodbusrtuovertcpclient_p.h \
    mb_rtu_over_tcp/qmodbusrtuovertcpclient.h \
    modbus_regs/modbus_regs.h \
    sysconfigs/sysconfigs.h \
    test_param_settings.h \
    main_dialog.h \
    test_params_struct.h \
    test_result_judge/test_result_judge.h \
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
