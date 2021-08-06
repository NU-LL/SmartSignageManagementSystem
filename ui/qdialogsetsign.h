#ifndef QDIALOGSETSIGN_H
#define QDIALOGSETSIGN_H

#include <QDialog>


#include "qformsigntable.h"


namespace Ui {
class QDialogSetSign;
}

class QDialogSetSign : public QDialog
{
    Q_OBJECT
private:
    Sign* sign = nullptr;

public:
    explicit QDialogSetSign(QWidget *parent = nullptr);
    ~QDialogSetSign();

    void setParameters(Sign* sign);
    void setIdEnabled(bool idEdit = true);//添加设备时 id可以设置 修改时不能设置

private slots:
    void on_buttonBox_accepted();

private:
    Ui::QDialogSetSign *ui;
};

#endif // QDIALOGSETSIGN_H
