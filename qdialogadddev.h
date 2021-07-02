#ifndef QDIALOGADDDEV_H
#define QDIALOGADDDEV_H

#include <QDialog>
#include "tcpserver.h"

namespace Ui {
class QDialogAddDev;
}

class QDialogAddDev : public QDialog
{
    Q_OBJECT

public:
    explicit QDialogAddDev(QWidget *parent = nullptr);
    ~QDialogAddDev();

    QString getName();
    QString getDeviceIp();
    QString getDeviceId();
    QString getSign();

private slots:
    void on_comboBox_Device_currentTextChanged(const QString &arg1);

private:
//    bool eventFilter(QObject *obj, QEvent *e);


private:
    Ui::QDialogAddDev *ui;
};

#endif // QDIALOGADDDEV_H
