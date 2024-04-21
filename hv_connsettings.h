#ifndef HV_CONNSETTINGS_H
#define HV_CONNSETTINGS_H

#include <QDialog>

namespace Ui {
class hvConnSettings;
}

class hvConnSettings : public QDialog
{
    Q_OBJECT

public:
    explicit hvConnSettings(QWidget *parent = nullptr);
    ~hvConnSettings();

private:
    Ui::hvConnSettings *ui;
};

#endif // HV_CONNSETTINGS_H
