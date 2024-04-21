﻿#include "main_dialog.h"
#include "logger/logger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    if(QT_VERSION>=QT_VERSION_CHECK(5,6,0))
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    Dialog w;
    QThread th;

    start_log_thread(th);

    w.show();

    end_log_thread(th);
    return a.exec();
}
