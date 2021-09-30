#ifndef QDLGABOUT_H
#define QDLGABOUT_H

#include <QDialog>

namespace Ui {
class QDlgAbout;
}

class QDlgAbout : public QDialog
{
    Q_OBJECT

public:
    explicit QDlgAbout(QWidget *parent = nullptr);
    ~QDlgAbout();

private slots:
    void on_pushButton_Details_clicked();

private:
    Ui::QDlgAbout *ui;
};

#endif // QDLGABOUT_H
