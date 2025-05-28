QT       += core gui serialbus serialport network charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += "$$[QT_INSTALL_HEADERS]/QtSerialBus/$$[QT_VERSION]/QtSerialBus" \
"$$[QT_INSTALL_HEADERS]/QtCore/$$[QT_VERSION]/QtCore" \
"$$[QT_INSTALL_HEADERS]/QtCore/$$[QT_VERSION]"

TARGET = X-scan-tester
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common_tools/common_tool_func.cpp \
    config_recorder/uiconfigrecorder.cpp \
    curveplotwidget.cpp \
    grayimgdisplay.cpp \
    hv_connsettings.cpp \
    hv_tester/hvtester.cpp \
    logger/logger.cpp \
    mb_regs_chart_display.cpp \
    mb_rtu_over_tcp/qmodbusrtuovertcpclient.cpp \
    modbus_regs/modbus_regs.cpp \
    pb_set_and_monitor.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    qtsingleapplication/qtlockedfile.cpp \
    qtsingleapplication/qtlockedfile_unix.cpp \
    qtsingleapplication/qtlockedfile_win.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    qtsingleapplication/qtsinglecoreapplication.cpp \
    recvscanneddata.cpp \
    sc_data_connsettings.cpp \
    sc_data_proc.cpp \
    serialportsetdlg.cpp \
    sysconfigs/sysconfigs.cpp \
    test_param_settings.cpp \
    main.cpp \
    main_dialog.cpp \
    test_result_judge/test_result_judge.cpp \
    version_def/version_def.cpp

HEADERS += \
    common_tools/common_tool_func.h \
    config_recorder/uiconfigrecorder.h \
    curveplotwidget.h \
    grayimgdisplay.h \
    hv_connsettings.h \
    hv_tester/hvtester.h \
    logger/logger.h \
    mb_regs_chart_display.h \
    mb_rtu_over_tcp/private/qmodbusrtuovertcpclient_p.h \
    mb_rtu_over_tcp/qmodbusrtuovertcpclient.h \
    modbus_regs/modbus_regs.h \
    qtsingleapplication/QtLockedFile \
    qtsingleapplication/QtSingleApplication \
    qtsingleapplication/qtlocalpeer.h \
    qtsingleapplication/qtlockedfile.h \
    qtsingleapplication/qtsingleapplication.h \
    qtsingleapplication/qtsinglecoreapplication.h \
    recvscanneddata.h \
    sc_data_connsettings.h \
    sc_data_proc.h \
    serialportsetdlg.h \
    sysconfigs/sysconfigs.h \
    test_param_settings.h \
    main_dialog.h \
    test_params_struct.h \
    test_result_judge/test_result_judge.h \
    version_def/version_def.h

FORMS += \
    grayimgdisplay.ui \
    hv_connsettings.ui \
    main_dialog.ui \
    sc_data_connsettings.ui \
    serialportsetdlg.ui \
    test_param_settings.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
msvc:QMAKE_CXXFLAGS += -execution-charset:utf-8

RC_FILE = X-exposer.rc

RESOURCES += \
    X-exposer.qrc
