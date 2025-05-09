#ifndef CURVEPLOTWIDGET_H
#define CURVEPLOTWIDGET_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

class CurvePlotWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit CurvePlotWidget(QWidget *parent = nullptr);
    void receiveData(int pt_cnt, const QVector<quint32>& row_data, quint32 range_max);
    void clearAllSeries();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QChart *m_chart;
    QChartView *m_chartView;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    int m_seriesIndex = 0;
};

#endif // CURVEPLOTWIDGET_H
