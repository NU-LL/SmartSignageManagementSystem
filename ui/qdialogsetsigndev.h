#ifndef QDIALOGSETSIGNDEV_H
#define QDIALOGSETSIGNDEV_H

#include <QDialog>


#include "ui/qformmain.h"



namespace Ui {
class QDialogSetSignDev;
}

class QDialogSetSignDev : public QDialog
{
    Q_OBJECT
private:
    TcpSignDevice* dev = nullptr;

public:
    enum mode{
        DEFAULT_MODE,
        BATCH_MODE,
        VOICE_MODE,
        FLASH_MODE,
        ALERT_MODE,
        SIGN_MODE,
        GROUP_MODE,
    };



    explicit QDialogSetSignDev(QWidget *parent = nullptr, mode batchMode = DEFAULT_MODE);
    ~QDialogSetSignDev();

    void setParameters(TcpSignDevice* signdev);




    mode batchMode = DEFAULT_MODE;//批量修改模式



private slots:
    void on_buttonBox_accepted();

private:
    Ui::QDialogSetSignDev *ui;
};

#endif // QDIALOGSETSIGNDEV_H
