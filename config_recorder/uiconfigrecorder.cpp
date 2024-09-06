#include <QSettings>
#include <QList>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>

#include "uiconfigrecorder.h"
#include "logger/logger.h"

UiConfigRecorder::UiConfigRecorder(QObject *parent, QString cfg_file_fpn)
    : QObject{parent}, m_cfg_file_fpn(cfg_file_fpn)
{
    if(cfg_file_fpn.isEmpty())
    {
        DIY_LOG(LOG_WARN, "the name of config file for recording ui configrations is empty.\n");
        return;
    }
}

#define LIST_VAR_NAME(ctrl) ctrl##_ptr_list
#define DEF_CTRL(ctrl)  QList<ctrl*> LIST_VAR_NAME(ctrl);

#define CTRL_PTR_VAR_DEF \
    DEF_CTRL(QLineEdit)\
    DEF_CTRL(QTextEdit)\
    DEF_CTRL(QComboBox)\
    DEF_CTRL(QRadioButton)\
    DEF_CTRL(QCheckBox)
#define BOX_CHECKED 1
#define BOX_UNCHECKED 0

#define CTRL_WRITE_TO_CFG(ctrl_type, value) \
    LIST_VAR_NAME(ctrl_type) = ui_widget->findChildren<ctrl_type*>();\
    for(idx = 0; idx < LIST_VAR_NAME(ctrl_type).size(); ++idx)        \
    {                                                                 \
        cfg_setting.setValue(LIST_VAR_NAME(ctrl_type)[idx]->objectName(),\
                             LIST_VAR_NAME(ctrl_type)[idx]->value);  \
    }

void UiConfigRecorder::record_ui_configs(QWidget * ui_widget, QSettings::Format cfg_format)
{
    CTRL_PTR_VAR_DEF;
    int idx;
    QSettings cfg_setting(m_cfg_file_fpn, cfg_format);

    if(!ui_widget)
    {
        DIY_LOG(LOG_ERROR, "ui_widget is NULL.\n");
        return;
    }

    cfg_setting.beginGroup(ui_widget->objectName());

    CTRL_WRITE_TO_CFG(QLineEdit, text())
    CTRL_WRITE_TO_CFG(QTextEdit, toPlainText())
    CTRL_WRITE_TO_CFG(QComboBox, currentIndex())
    CTRL_WRITE_TO_CFG(QRadioButton, isChecked() ? BOX_CHECKED : BOX_UNCHECKED)
    CTRL_WRITE_TO_CFG(QCheckBox, isChecked() ? BOX_CHECKED : BOX_UNCHECKED)

    cfg_setting.endGroup();
}

#define READ_FROM_CFG(ctrl_type, val, cond, op) \
    LIST_VAR_NAME(ctrl_type) = ui_widget->findChildren<ctrl_type *>();\
    for(idx = 0; idx < LIST_VAR_NAME(ctrl_type).size(); ++idx)\
    {\
        str_val = cfg_setting.value(LIST_VAR_NAME(ctrl_type)[idx]->objectName(), "").toString();\
        val;\
        if(cond)\
        {\
            LIST_VAR_NAME(ctrl_type)[idx]->op;\
        }\
    }

void UiConfigRecorder::load_configs_to_ui(QWidget * ui_widget, QSettings::Format cfg_format)
{
    CTRL_PTR_VAR_DEF;
    int idx;
    QSettings cfg_setting(m_cfg_file_fpn, cfg_format);
    QString str_val;
    int int_val;
    bool tr_ret;

    if(!ui_widget)
    {
        DIY_LOG(LOG_ERROR, "ui_widget is NULL.\n");
        return;
    }

    cfg_setting.beginGroup(ui_widget->objectName());

    READ_FROM_CFG(QLineEdit, , true, setText(str_val));
    READ_FROM_CFG(QTextEdit, , true, setText(str_val));
    READ_FROM_CFG(QComboBox, int_val = str_val.toInt(&tr_ret),
                  (tr_ret && (int_val < LIST_VAR_NAME(QComboBox)[idx]->count())),
                  setCurrentIndex(int_val));
    READ_FROM_CFG(QRadioButton, int_val = str_val.toInt(&tr_ret),
                  (tr_ret && (BOX_CHECKED == int_val || BOX_UNCHECKED == int_val)),
                  setChecked(int_val));
    READ_FROM_CFG(QCheckBox, int_val = str_val.toInt(&tr_ret),
                  (tr_ret && (BOX_CHECKED == int_val || BOX_UNCHECKED == int_val)),
                  setChecked(int_val));

    cfg_setting.endGroup();
}
