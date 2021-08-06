#include "qdlgabout.h"
#include "ui_qdlgabout.h"

QDlgAbout::QDlgAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QDlgAbout)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose);//设置为关闭时删除
}

QDlgAbout::~QDlgAbout()
{
    delete ui;
}
