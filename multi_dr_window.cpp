#include "multi_dr_window.h"
#include "main_dialog.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QUrl>

namespace {
const QString settings_file = ".user_settings.ini";
const QString device_list_key = "MultiDr/devices";
const QString legacy_device_id = "DR-01";
const QRegularExpression valid_device_id("^[A-Za-z0-9_-]{1,32}$");
enum Column { Pick, Device, Endpoint, Connection, Status, Progress, Result, ColumnCount };
}

MultiDrWindow::MultiDrWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/icons/app_images/app.ico"));
    resize(1050, 560);

    auto central = new QWidget(this);
    auto page = new QVBoxLayout(central);
    auto toolbar = new QHBoxLayout;
    auto add = new QPushButton("添加设备", central);
    add->setObjectName("addDeviceButton");
    m_delete = new QPushButton("删除设备", central);
    m_delete->setObjectName("deleteDeviceButton");
    m_batch_start = new QPushButton("开始所选测试", central);
    m_batch_start->setObjectName("startSelectedDevicesButton");
    m_select_all = new QCheckBox("全选可启动设备", central);
    m_summary = new QLabel(central);
    toolbar->addWidget(add);
    toolbar->addWidget(m_delete);
    toolbar->addSpacing(12);
    toolbar->addWidget(m_select_all);
    toolbar->addWidget(m_batch_start);
    toolbar->addStretch();
    toolbar->addWidget(m_summary);
    page->addLayout(toolbar);

    auto body = new QHBoxLayout;
    m_table = new QTableWidget(0, ColumnCount, central);
    m_table->setObjectName("drDeviceTable");
    m_table->setHorizontalHeaderLabels({"选择", "设备", "连接地址", "连接", "测试状态", "曝光进度", "最近结果"});
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Endpoint, QHeaderView::Stretch);
    body->addWidget(m_table, 1);

    auto detail = new QGroupBox("设备详情", central);
    m_detail = detail;
    auto detail_layout = new QVBoxLayout(detail);
    auto detail_form = new QFormLayout;
    m_detail_status = new QLabel(detail);
    m_detail_endpoint = new QLabel(detail);
    m_detail_endpoint->setWordWrap(true);
    m_detail_endpoint->setTextFormat(Qt::PlainText);
    m_detail_endpoint->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_detail_remaining = new QLabel(detail);
    detail_form->addRow("设备状态", m_detail_status);
    detail_form->addRow("连接参数", m_detail_endpoint);
    detail_form->addRow("预计剩余", m_detail_remaining);
    detail_layout->addLayout(detail_form);

    auto settings_row = new QHBoxLayout;
    m_connection_settings = new QPushButton("高压连接设置", detail);
    m_test_settings = new QPushButton("测试参数设置", detail);
    settings_row->addWidget(m_connection_settings);
    settings_row->addWidget(m_test_settings);
    detail_layout->addLayout(settings_row);
    auto connection_row = new QHBoxLayout;
    m_connect = new QPushButton("连接", detail);
    m_disconnect = new QPushButton("断开", detail);
    connection_row->addWidget(m_connect);
    connection_row->addWidget(m_disconnect);
    detail_layout->addLayout(connection_row);
    auto test_row = new QHBoxLayout;
    m_start = new QPushButton("开始测试", detail);
    m_pause = new QPushButton("暂停", detail);
    m_stop = new QPushButton("停止测试", detail);
    test_row->addWidget(m_start);
    test_row->addWidget(m_pause);
    test_row->addWidget(m_stop);
    detail_layout->addLayout(test_row);
    auto inspect_row = new QHBoxLayout;
    auto details_button = new QPushButton("打开详情与监控", detail);
    m_open_report = new QPushButton("打开最近报告", detail);
    inspect_row->addWidget(details_button);
    inspect_row->addWidget(m_open_report);
    detail_layout->addLayout(inspect_row);
    m_detail_message = new QLabel(detail);
    m_detail_message->setTextFormat(Qt::PlainText);
    m_detail_message->setWordWrap(true);
    detail_layout->addWidget(m_detail_message);
    detail_layout->addStretch();
    detail->setMinimumWidth(275);
    detail->setMaximumWidth(340);
    body->addWidget(detail);
    page->addLayout(body, 1);
    setCentralWidget(central);

    connect(add, &QPushButton::clicked, this, &MultiDrWindow::promptAddDevice);
    connect(m_delete, &QPushButton::clicked, this, &MultiDrWindow::promptDeleteDevice);
    connect(m_batch_start, &QPushButton::clicked, this, &MultiDrWindow::startSelectedDevices);
    connect(m_table, &QTableWidget::currentCellChanged, this, &MultiDrWindow::refreshDetails);
    connect(m_table, &QTableWidget::itemChanged, this, &MultiDrWindow::refreshBatchControls);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &MultiDrWindow::showDeviceDetails);
    connect(details_button, &QPushButton::clicked, this, &MultiDrWindow::showDeviceDetails);
    connect(m_select_all, &QCheckBox::clicked, this, [this](bool checked) {
        const QSignalBlocker updating(m_table);
        for(int row = 0; row < m_devices.size(); ++row)
        {
            if(m_devices.at(row)->canStart())
            {
                m_table->item(row, Pick)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
            }
        }
        refreshBatchControls();
    });
    connect(m_connection_settings, &QPushButton::clicked, this, [this]() {
        if(auto device = selectedDevice()) device->on_hvConnSetBtn_clicked();
    });
    connect(m_test_settings, &QPushButton::clicked, this, [this]() {
        if(auto device = selectedDevice()) device->on_testParamSetBtn_clicked();
    });
    connect(m_connect, &QPushButton::clicked, this, [this]() {
        if(auto device = selectedDevice()) device->on_hvConnBtn_clicked();
    });
    connect(m_disconnect, &QPushButton::clicked, this, [this]() {
        if(auto device = selectedDevice()) device->on_hvDisconnBtn_clicked();
    });
    connect(m_start, &QPushButton::clicked, this, [this]() {
        if(auto device = selectedDevice())
        {
            const QString error = device->startTest();
            if(!error.isEmpty()) QMessageBox::warning(this, device->deviceId(), error);
        }
    });
    connect(m_pause, &QPushButton::clicked, this, [this]() {
        if(auto device = selectedDevice()) device->on_pauseTestBtn_clicked();
    });
    connect(m_stop, &QPushButton::clicked, this, [this]() {
        if(auto device = selectedDevice())
        {
            if(QMessageBox::question(this, "停止测试",
                    QString("停止 %1 的测试？已启动的曝光将自然结束。").arg(device->deviceId())) == QMessageBox::Yes)
            {
                device->on_stopTestBtn_clicked();
            }
        }
    });
    connect(m_open_report, &QPushButton::clicked, this, [this]() {
        auto device = selectedDevice();
        if(!device || device->lastReport().isEmpty()) return;
        const QString path = device->lastReport();
        if(!QFileInfo::exists(path) || !QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        {
            QMessageBox::warning(this, "打开报告", QString("无法打开报告。\n%1").arg(path));
        }
    });

    const QSettings settings(settings_file, QSettings::IniFormat);
    const auto devices = settings.value(device_list_key, QStringList{legacy_device_id}).toStringList();
    for(const auto &id : devices)
    {
        if(valid_device_id.match(id).hasMatch()) addDevice(id);
    }
    if(!m_devices.isEmpty()) m_table->setCurrentCell(0, Device);
    refreshBatchControls();
    refreshDetails();
}

MultiDrWindow::~MultiDrWindow()
{
    for(auto device : m_devices)
    {
        device->disconnect(this);
        device->reject();
        delete device;
    }
}

bool MultiDrWindow::hasDevice(const QString &id) const
{
    for(const auto device : m_devices)
    {
        if(id.compare(device->deviceId(), Qt::CaseInsensitive) == 0) return true;
    }
    return false;
}

void MultiDrWindow::addDevice(const QString &id)
{
    if(hasDevice(id)) return;
    const QString group = id == legacy_device_id ? QString() : QString("MultiDr/profiles/%1/").arg(id);
    auto device = new Dialog(id, settings_file, group, this);
    device->setConnectionCheck([this](const Dialog &target) { return checkConnection(target); });
    const int row = m_devices.size();
    m_devices.append(device);
    m_table->insertRow(row);
    for(int column = 0; column < ColumnCount; ++column)
    {
        auto item = new QTableWidgetItem;
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        if(column == Pick) item->setCheckState(Qt::Unchecked);
        m_table->setItem(row, column, item);
    }
    auto progress = new QProgressBar(m_table);
    progress->setRange(0, 100);
    progress->setAlignment(Qt::AlignCenter);
    progress->setAccessibleName(QString("%1 曝光进度").arg(id));
    m_table->setCellWidget(row, Progress, progress);
    connect(device, &Dialog::deviceChanged, this, [this, device]() {
        const int current_row = m_devices.indexOf(device);
        if(current_row >= 0) refreshRow(current_row);
    });
    refreshRow(row);
}

void MultiDrWindow::promptAddDevice()
{
    int number = m_devices.size() + 1;
    QString suggested;
    do
    {
        suggested = QString("DR-%1").arg(number++, 2, 10, QLatin1Char('0'));
    } while(hasDevice(suggested));
    bool accepted = false;
    const QString id = QInputDialog::getText(this, "添加设备", "设备编号",
                       QLineEdit::Normal, suggested, &accepted).trimmed();
    if(!accepted) return;
    if(!valid_device_id.match(id).hasMatch() || hasDevice(id))
    {
        QMessageBox::warning(this, "添加设备", "设备编号必须唯一，可输入 1 至 32 个字母、数字、横线或下划线。");
        return;
    }

    addDevice(id);
    if(!saveDeviceList())
    {
        QMessageBox::warning(this, "保存设备列表", "设备列表保存失败。本次运行仍可使用新设备。");
    }
    m_table->setCurrentCell(m_devices.size() - 1, Device);
    m_devices.last()->on_hvConnSetBtn_clicked();
}

void MultiDrWindow::promptDeleteDevice()
{
    auto device = selectedDevice();
    if(!device || device->isTesting()) return;

    const QString message = QString("删除 %1 及其连接和测试设置？已生成的 CSV 会保留。")
                            .arg(device->deviceId());
    if(QMessageBox::question(this, "删除设备", message,
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }
    if(device->isTesting()) return;
    if(!saveDeviceList(device))
    {
        QMessageBox::warning(this, "删除设备", "无法保存设备列表。请检查设置文件是否可写。");
        return;
    }

    const int row = m_devices.indexOf(device);
    device->disconnect(this);
    m_devices.removeAt(row);
    {
        const QSignalBlocker updating(m_table);
        m_table->removeRow(row);
        if(!m_devices.isEmpty()) m_table->setCurrentCell(qMin(row, m_devices.size() - 1), Device);
    }
    delete device;
    refreshBatchControls();
    refreshDetails();
}

bool MultiDrWindow::saveDeviceList(const Dialog *removed) const
{
    QStringList ids;
    for(const auto device : m_devices)
    {
        if(device != removed) ids << device->deviceId();
    }
    QSettings settings(settings_file, QSettings::IniFormat);
    settings.setValue(device_list_key, ids);
    if(removed)
    {
        const QString id = removed->deviceId();
        if(id == legacy_device_id)
        {
            settings.remove("Dialog");
            settings.remove("hvConnSettings");
            settings.remove("testParamSettingsDialog");
        }
        else
        {
            settings.remove(QString("MultiDr/profiles/%1").arg(id));
        }
    }
    settings.sync();
    return settings.status() == QSettings::NoError;
}

Dialog *MultiDrWindow::selectedDevice() const
{
    const int row = m_table->currentRow();
    return row >= 0 && row < m_devices.size() ? m_devices.at(row) : nullptr;
}

void MultiDrWindow::showDeviceDetails()
{
    if(auto device = selectedDevice())
    {
        device->showNormal();
        device->raise();
        device->activateWindow();
    }
}

void MultiDrWindow::refreshRow(int row)
{
    const auto device = m_devices.at(row);
    {
        const QSignalBlocker updating(m_table);
        auto pick = m_table->item(row, Pick);
        if(!device->canStart()) pick->setCheckState(Qt::Unchecked);
        auto flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if(device->canStart()) flags |= Qt::ItemIsUserCheckable;
        pick->setFlags(flags);
        m_table->item(row, Device)->setText(device->deviceId());
        m_table->item(row, Endpoint)->setText(device->endpoint());
        m_table->item(row, Connection)->setText(device->connectionStatus());
        m_table->item(row, Status)->setText(device->testStatus());
        m_table->item(row, Progress)->setText(device->progressText());
        auto progress = static_cast<QProgressBar *>(m_table->cellWidget(row, Progress));
        progress->setValue(device->progressPercent());
        progress->setFormat(device->progressText());
        m_table->item(row, Result)->setText(device->lastResult());
        m_table->item(row, Result)->setToolTip(device->lastReport());
    }
    refreshBatchControls();
    if(m_table->currentRow() == row) refreshDetails();
}

void MultiDrWindow::refreshDetails()
{
    const auto device = selectedDevice();
    m_delete->setEnabled(device && !device->isTesting());
    m_detail->setEnabled(device != nullptr);
    if(!device)
    {
        m_detail_status->setText("请选择设备");
        m_detail_endpoint->clear();
        m_detail_remaining->clear();
        m_detail_message->clear();
        return;
    }
    m_detail_status->setText(QString("%1 · %2 · %3")
                             .arg(device->deviceId(), device->connectionStatus(), device->testStatus()));
    m_detail_endpoint->setText(device->connectionInfo());
    m_detail_remaining->setText(device->remainingTime());
    m_detail_message->setText(device->lastMessage());
    m_connection_settings->setEnabled(!device->endpointInUse());
    m_test_settings->setEnabled(!device->isTesting());
    m_connect->setEnabled(device->canConnect());
    m_disconnect->setEnabled(device->canDisconnect());
    m_start->setEnabled(device->canStart());
    m_pause->setEnabled(device->isTesting());
    m_pause->setText(device->isPaused() ? "恢复" : "暂停");
    m_stop->setEnabled(device->isTesting());
    m_open_report->setEnabled(!device->lastReport().isEmpty());
}

void MultiDrWindow::refreshBatchControls()
{
    int eligible = 0;
    int picked = 0;
    int testing = 0;
    for(int row = 0; row < m_devices.size(); ++row)
    {
        const auto device = m_devices.at(row);
        testing += device->isTesting();
        if(!device->canStart()) continue;
        ++eligible;
        const auto pick = m_table->item(row, Pick);
        if(pick && pick->checkState() == Qt::Checked) ++picked;
    }
    m_select_all->setEnabled(eligible > 0);
    m_select_all->setChecked(eligible > 0 && eligible == picked);
    m_batch_start->setEnabled(picked > 0);
    m_summary->setText(QString("%1 台设备 · %2 台有测试任务 · 已选 %3 台")
                       .arg(m_devices.size()).arg(testing).arg(picked));
}

void MultiDrWindow::startSelectedDevices()
{
    QVector<Dialog *> selected;
    for(int row = 0; row < m_devices.size(); ++row)
    {
        if(m_table->item(row, Pick)->checkState() == Qt::Checked)
        {
            selected.append(m_devices.at(row));
        }
    }
    QStringList errors;
    for(auto device : selected)
    {
        const QString error = device->startTest();
        if(!error.isEmpty()) errors << QString("%1：%2").arg(device->deviceId(), error);
    }
    if(!errors.isEmpty()) QMessageBox::warning(this, "部分设备未启动", errors.join('\n'));
}

QString MultiDrWindow::checkConnection(const Dialog &target) const
{
    const QString key = target.endpointKey();
    if(key.isEmpty()) return {};
    for(const auto device : m_devices)
    {
        if(device != &target && device->endpointKey() == key)
        {
            return QString("设备 %1 已配置连接地址 %2。请为 %3 配置独立的连接地址。")
                    .arg(device->deviceId(), device->endpoint(), target.deviceId());
        }
    }
    return {};
}

void MultiDrWindow::closeEvent(QCloseEvent *event)
{
    QStringList testing;
    for(const auto device : m_devices)
    {
        if(device->isTesting()) testing << device->deviceId();
    }
    if(!testing.isEmpty() && QMessageBox::question(this, "退出 X-exposer",
            QString("退出将停止这些测试：%1。已启动的曝光将自然结束。").arg(testing.join("、")),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    {
        event->ignore();
        return;
    }
    for(auto device : m_devices)
    {
        device->on_stopTestBtn_clicked();
        device->reject();
    }
    QMainWindow::closeEvent(event);
}
