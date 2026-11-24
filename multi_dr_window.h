#ifndef MULTI_DR_WINDOW_H
#define MULTI_DR_WINDOW_H

#include <QMainWindow>
#include <QVector>

class Dialog;
class QLabel;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QTableWidget;

class MultiDrWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MultiDrWindow(QWidget *parent = nullptr);
    ~MultiDrWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QVector<Dialog *> m_devices;
    QTableWidget *m_table;
    QGroupBox *m_detail;
    QLabel *m_summary, *m_detail_status, *m_detail_endpoint, *m_detail_remaining, *m_detail_message;
    QCheckBox *m_select_all;
    QPushButton *m_delete, *m_batch_start, *m_connection_settings, *m_test_settings, *m_connect, *m_disconnect;
    QPushButton *m_start, *m_pause, *m_stop, *m_open_report;

    void addDevice(const QString &id);
    bool hasDevice(const QString &id) const;
    void promptAddDevice();
    void promptDeleteDevice();
    bool saveDeviceList(const Dialog *removed = nullptr) const;
    Dialog *selectedDevice() const;
    void showDeviceDetails();
    void refreshRow(int row);
    void refreshDetails();
    void refreshBatchControls();
    void startSelectedDevices();
    QString checkConnection(const Dialog &target) const;
};

#endif // MULTI_DR_WINDOW_H
