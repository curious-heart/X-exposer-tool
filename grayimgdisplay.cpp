#include <QFileDialog>

#include "common_tools/common_tool_func.h"
#include "grayimgdisplay.h"
#include "ui_grayimgdisplay.h"

static const char* gs_str_save_file = "保存文件";

const char* g_str_real_img_wnd_str = "real_img";
const char* g_str_layfull_img_wnd_str = "layfull_img";

GrayImgDisplay::GrayImgDisplay(QWidget *parent, UiConfigRecorder * cfg_recorder, QString wnd_str) :
    QMainWindow(parent),
    ui(new Ui::GrayImgDisplay),
    m_cfg_recorder(cfg_recorder),
    m_wnd_str(wnd_str)
{
    ui->setupUi(this);

    ui->hiddenDirRecLEdit->setVisible(false);

    ui->grayImgLbl->setAlignment(Qt::AlignCenter);
    ui->grayImgLbl->setScaledContents(false);
    ui->grayImgLbl->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);


    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();
    if(m_cfg_recorder) m_cfg_recorder->load_configs_to_ui(this,
                                                          m_rec_ui_cfg_fin, m_rec_ui_cfg_fout,
                                                          m_wnd_str);

    if(!m_wnd_str.isEmpty()) setWindowTitle(m_wnd_str);
}

GrayImgDisplay::~GrayImgDisplay()
{
    delete ui;
}

void GrayImgDisplay::set_img_to_lbl()
{
    if (!m_local_img.isNull())
    {
        QPixmap scaled = QPixmap::fromImage(m_local_img_8bit)
                    .scaled(ui->grayImgLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->grayImgLbl->setPixmap(scaled);
    }
}

void GrayImgDisplay::update_img_display(QImage &img)
{
    m_local_img = img;
    m_local_img_8bit = convertGrayscale16To8(m_local_img);

    set_img_to_lbl();
}

void GrayImgDisplay::on_saveImgPbt_clicked()
{
    static QString ls_img_file_filter = "Images (*.png *.bmp *.jpg)";
    static QString ls_8bit_fpn_suffix = "-8bit";

    QString img_file_fpn;
    QString dir_str = ui->hiddenDirRecLEdit->text();

    if(dir_str.isEmpty()) dir_str = ".";
    img_file_fpn = QFileDialog::getSaveFileName(this, gs_str_save_file,
                                                dir_str, ls_img_file_filter);

    QFileInfo img_fpn_info(img_file_fpn);
    QString img_8bit_file_fpn = img_fpn_info.path() + "/" +
                                img_fpn_info.baseName() + ls_8bit_fpn_suffix + "." + img_fpn_info.suffix();

    if(!img_file_fpn.isEmpty())
    {
        m_local_img.save(img_file_fpn);
        m_local_img_8bit.save(img_8bit_file_fpn);

        ui->hiddenDirRecLEdit->setText(QFileInfo(img_file_fpn).path());
        if(m_cfg_recorder) m_cfg_recorder->record_ui_configs(this,
                                                     m_rec_ui_cfg_fin, m_rec_ui_cfg_fout,
                                                     m_wnd_str);
    }

}

void GrayImgDisplay::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    set_img_to_lbl();
}
