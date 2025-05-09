
#include "curveplotwidget.h"
#include <QVBoxLayout>
#include <QCloseEvent>

CurvePlotWidget::CurvePlotWidget(QWidget *parent)
    : QMainWindow(parent)
{
    m_chart = new QChart();
    m_chart->setTitle("Multiple Series Curve Plot");
    m_chart->legend()->setVisible(true);

    m_axisX = new QValueAxis();
    m_axisX->setTitleText("Index");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    m_axisY = new QValueAxis();
    m_axisY->setTitleText("Value");
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    setCentralWidget(m_chartView);
}

void CurvePlotWidget::clearAllSeries()
{
    m_chart->removeAllSeries();
    m_seriesIndex = 0;
}

void CurvePlotWidget::receiveData(int pt_cnt, const QVector<quint32>& row_data, quint32 range_max)
{
    if (pt_cnt > row_data.size())
        return;

    QLineSeries *series = new QLineSeries();
    series->setName(QString("Series %1").arg(++m_seriesIndex));

    for (int i = 0; i < pt_cnt; ++i)
        series->append(i, row_data[i]);

    m_chart->addSeries(series);
    series->attachAxis(m_axisX);
    series->attachAxis(m_axisY);

    m_axisX->setRange(0, pt_cnt - 1);
    m_axisY->setRange(0, range_max);
}

void CurvePlotWidget::closeEvent(QCloseEvent *event)
{
    event->ignore();  // 阻止真正关闭
    this->hide();     // 改为隐藏
}
