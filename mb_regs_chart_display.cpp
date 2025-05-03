#include "mb_regs_chart_display.h"


extern const char* g_str_cube_volt;
extern const char* g_str_cube_current;
extern const char* g_str_volt_unit_kv;
extern const char* g_str_current_unit_a;
extern const char* g_str_current_unit_ma;
extern const char* g_str_current_unit_ua;
extern const char* g_str_time;

static const int gs_def_volt_axis_low = 0, gs_def_volt_axis_up = 250;
static const float gs_def_current_axis_low = 0, gs_def_current_axis_up = 200;
static const int gs_def_time_axis_pt_cnt = 100;

#define VOLT_TITLE_TEXT m_volt_name_str + "(" + m_volt_unit_str + ")"
#define CURRENT_TITLE_TEXT m_current_name_str + "(" + m_current_unit_str + ")"

MbRegsChartDisp::MbRegsChartDisp(QWidget *parent)
    : QMainWindow(parent),
      m_volt_axis_low(gs_def_volt_axis_low), m_volt_axis_up(gs_def_volt_axis_up),
      m_current_axis_low(gs_def_current_axis_low), m_current_axis_up(gs_def_current_axis_up),
      m_time_axis_pt_cnt(gs_def_time_axis_pt_cnt),
      m_volt_unit_str(g_str_volt_unit_kv), m_current_unit_str(g_str_current_unit_ua),
      m_volt_name_str(g_str_cube_volt), m_current_name_str(g_str_cube_current),
      timeIndex(0)
{
    voltSeries = new QLineSeries();
    voltSeries->setName(VOLT_TITLE_TEXT);

    currentSeries = new QLineSeries();
    currentSeries->setName(CURRENT_TITLE_TEXT);

    QChart *chart = new QChart();
    chart->addSeries(voltSeries);
    chart->addSeries(currentSeries);
    chart->setTitle("");
    chart->legend()->setVisible(true);

    time_axis_X = new QValueAxis();
    time_axis_X->setTitleText(g_str_time);
    time_axis_X->setRange(0, m_time_axis_pt_cnt);
    chart->addAxis(time_axis_X, Qt::AlignBottom);
    voltSeries->attachAxis(time_axis_X);
    currentSeries->attachAxis(time_axis_X);

    volt_axis_Y = new QValueAxis();
    volt_axis_Y->setTitleText(VOLT_TITLE_TEXT);
    volt_axis_Y->setRange(m_volt_axis_low, m_volt_axis_up);
    chart->addAxis(volt_axis_Y, Qt::AlignLeft);
    voltSeries->attachAxis(volt_axis_Y);

    current_axis_Y = new QValueAxis();
    current_axis_Y->setTitleText(CURRENT_TITLE_TEXT);
    current_axis_Y->setRange(m_current_axis_low, m_current_axis_up);
    chart->addAxis(current_axis_Y, Qt::AlignRight);
    currentSeries->attachAxis(current_axis_Y);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(chartView);
}

void MbRegsChartDisp::addData(int volt, float current)
{
    voltSeries->append(timeIndex, volt);
    currentSeries->append(timeIndex, current);

    if (voltSeries->count() > m_time_axis_pt_cnt)
    {
        voltSeries->removePoints(0, voltSeries->count() - m_time_axis_pt_cnt);
        currentSeries->removePoints(0, currentSeries->count() - m_time_axis_pt_cnt);
        time_axis_X->setRange(timeIndex - m_time_axis_pt_cnt, timeIndex);
    }
    else
    {
        time_axis_X->setRange(0, m_time_axis_pt_cnt);
    }

    ++timeIndex;
}

void MbRegsChartDisp::set_volt_range(int low, int up)
{
    m_volt_axis_low = low;
    m_volt_axis_up = up;

    volt_axis_Y->setRange(low, up);
}

void MbRegsChartDisp::set_current_range(float low, float up)
{
    m_current_axis_low = low;
    m_current_axis_up = up;

    current_axis_Y->setRange(low, up);
}

void MbRegsChartDisp::set_time_axis_pt_cnt(int cnt)
{
    m_time_axis_pt_cnt = cnt;
}

void MbRegsChartDisp::set_volt_name_and_unit(QString n, QString u)
{
    m_volt_name_str = n; m_volt_unit_str = u;

    volt_axis_Y->setTitleText(VOLT_TITLE_TEXT);
    voltSeries->setName(VOLT_TITLE_TEXT);
}

void MbRegsChartDisp::set_current_name_and_unit(QString n, QString u)
{
    m_current_name_str = n; m_current_unit_str = u;

    current_axis_Y->setTitleText(CURRENT_TITLE_TEXT);
    currentSeries->setName(CURRENT_TITLE_TEXT);
}
