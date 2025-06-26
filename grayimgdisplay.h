#ifndef GRAYIMGDISPLAY_H
#define GRAYIMGDISPLAY_H

#include <QMainWindow>

#include "config_recorder/uiconfigrecorder.h"

namespace Ui {
class GrayImgDisplay;
}

class GrayImgDisplay : public QMainWindow
{
    Q_OBJECT

public:
    explicit GrayImgDisplay(QWidget *parent = nullptr, UiConfigRecorder * cfg_recorder = nullptr,
                            QString wnd_str = "");
    ~GrayImgDisplay();

    void update_img_display(QImage &img);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void set_img_to_lbl();

private slots:
    void on_saveImgPbt_clicked();

private:
    Ui::GrayImgDisplay *ui;

    UiConfigRecorder * m_cfg_recorder = nullptr;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    QImage m_local_img, m_local_img_8bit;
    QString m_wnd_str;
};

extern const char* g_str_real_img_wnd_str;
extern const char* g_str_layfull_img_wnd_str;

#endif // GRAYIMGDISPLAY_H
