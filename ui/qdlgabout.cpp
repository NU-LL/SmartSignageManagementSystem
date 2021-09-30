#include "qdlgabout.h"
#include "ui_qdlgabout.h"
#include <QMessageBox>
#include <QLocale>
#include <QDateTime>


QDlgAbout::QDlgAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QDlgAbout)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose);//设置为关闭时删除



    QString dateTime;
    dateTime += __DATE__;
    dateTime += __TIME__;
    dateTime.replace("  "," 0");//注意" "是两个空格，用于日期为单数时需要转成“空格+0”
    ui->label_BuiltDate->setText(QLocale(QLocale::English).toDateTime(dateTime, "MMM dd yyyyhh:mm:ss").toString("yyyy.MM.dd hh:mm:ss"));

    ui->label_Version->setText(qVersion());
}

QDlgAbout::~QDlgAbout()
{
    delete ui;
}

void QDlgAbout::on_pushButton_Details_clicked()
{
    QMessageBox::aboutQt(this, tr("关于QT"));
}

