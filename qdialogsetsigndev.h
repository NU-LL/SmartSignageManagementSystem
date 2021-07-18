#ifndef QDIALOGSETSIGNDEV_H
#define QDIALOGSETSIGNDEV_H

#include <QDialog>


#include "qformmain.h"



namespace Ui {
class QDialogSetSignDev;
}

class QDialogSetSignDev : public QDialog
{
    Q_OBJECT
private:
    SignDevice* signdev = nullptr;

public:
    explicit QDialogSetSignDev(QWidget *parent = nullptr);
    ~QDialogSetSignDev();


    void setParameters(SignDevice* signdev);

private slots:
    void on_buttonBox_accepted();

private:
    Ui::QDialogSetSignDev *ui;
};

#endif // QDIALOGSETSIGNDEV_H
