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
    QString getDevice();
    QString getSign();

private:
    Ui::QDialogAddDev *ui;
};

#endif // QDIALOGADDDEV_H
