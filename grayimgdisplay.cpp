#include "grayimgdisplay.h"
#include "ui_grayimgdisplay.h"

GrayImgDisplay::GrayImgDisplay(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GrayImgDisplay)
{
    ui->setupUi(this);
}

GrayImgDisplay::~GrayImgDisplay()
{
    delete ui;
}
