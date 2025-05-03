#ifndef REALTIMECHARTWINDOW_H
#define REALTIMECHARTWINDOW_H

#include <QString>
#include <QMainWindow>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

QT_CHARTS_USE_NAMESPACE

class MbRegsChartDisp: public QMainWindow
{
    Q_OBJECT

public:
    explicit MbRegsChartDisp(QWidget *parent = nullptr);
    void addData(int volt, float current);  // 新增数据接口

    void set_volt_range(int low, int up);
    void set_current_range(float low, float up);
    void set_time_axis_pt_cnt(int cnt);
    void set_volt_name_and_unit(QString n, QString u);
    void set_current_name_and_unit(QString n, QString u);

private:
    int m_volt_axis_low, m_volt_axis_up;
    float m_current_axis_low, m_current_axis_up;
    int m_time_axis_pt_cnt;
    QString m_volt_unit_str, m_current_unit_str;
    QString m_volt_name_str, m_current_name_str;

    QLineSeries *voltSeries = nullptr;
    QLineSeries *currentSeries = nullptr;
    QValueAxis *time_axis_X = nullptr;
    QValueAxis *volt_axis_Y = nullptr;
    QValueAxis *current_axis_Y = nullptr;
    int timeIndex;  // 时间索引
};

#endif // REALTIMECHARTWINDOW_H
