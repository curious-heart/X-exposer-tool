#include "multi_dr_window.h"
#include "version_def/version_def.h"
#include "logger/logger.h"
#include "sysconfigs/sysconfigs.h"

#include <QMessageBox>
#include <QString>
#include <QNetworkProxy>
#include "qtsingleapplication/qtsingleapplication.h"

int main(int argc, char *argv[])
{
    if(QT_VERSION>=QT_VERSION_CHECK(5,6,0))
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    static const char* ls_app_id_str = "x-exposer(a small tool for automatic test and aging test)";
    static const char* ls_hint_str = "程序已经启动";
    QtSingleApplication a(QString(ls_app_id_str), argc, argv);

    if(a.isRunning())
    {
        return !a.sendMessage(QString(ls_hint_str));
    }

    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    /*initialization: start log thread. Now use the default log-level defined in logger mode.*/
    QThread th;
    int ret;
    start_log_thread(th);

    /*initialization: load configs.*/
    QString ret_str;
    bool log_cfg_ret = fill_sys_configs(&ret_str);
    if(!log_cfg_ret)
    {
        end_log_thread(th);

        QMessageBox::critical(nullptr, "", ret_str);
        return -1;
    }
    //use the log level defined in config file.
    update_log_level((LOG_LEVEL)(g_sys_configs_block.log_level));

    {
        MultiDrWindow window;
        window.setWindowTitle(QString("X-exposer %1 - 多设备曝光测试").arg(APP_VER_STR));
        window.show();
        a.setActivationWindow(&window);
        QObject::connect(&a, &QtSingleApplication::messageReceived, &window, [&window](const QString &message) {
            QMessageBox::information(&window, "X-exposer", message);
        });
        ret = a.exec();
    }

    end_log_thread(th);
    return ret;
}
