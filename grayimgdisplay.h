#ifndef GRAYIMGDISPLAY_H
#define GRAYIMGDISPLAY_H

#include <QMainWindow>

namespace Ui {
class GrayImgDisplay;
}

class GrayImgDisplay : public QMainWindow
{
    Q_OBJECT

public:
    explicit GrayImgDisplay(QWidget *parent = nullptr);
    ~GrayImgDisplay();

private:
    Ui::GrayImgDisplay *ui;
};

#endif // GRAYIMGDISPLAY_H
