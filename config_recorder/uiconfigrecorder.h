#ifndef UICONFIGRECORDER_H
#define UICONFIGRECORDER_H

#include <QObject>
#include <QSettings>
#include <QString>

class UiConfigRecorder : public QObject
{
    Q_OBJECT
private:
    static constexpr const char * const m_def_cfg_file_fpn = "ui_cfg_recorder.ini";
    QString m_cfg_file_fpn;
public:
    explicit UiConfigRecorder(QObject *parent = nullptr,
                              QString cfg_file_fpn = m_def_cfg_file_fpn);

    void record_ui_configs(QWidget * ui_widget,
                           QSettings::Format cfg_format = QSettings::IniFormat);
    void load_configs_to_ui(QWidget * ui_widget,
                            QSettings::Format cfg_format = QSettings::IniFormat);

signals:

};

#endif // UICONFIGRECORDER_H
