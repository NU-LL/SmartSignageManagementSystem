#include "qformoptions.h"
#include "ui_qformoptions.h"
#include "config.h"

#include <QDir>
#include <QFileDialog>

QFormOptions::QFormOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormOptions)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除

//    ui->lineEdit_Path->setText(QDir::currentPath());
    ui->lineEdit_Path->setText(QDir(DEFAULT_PROFILE_PATH).absolutePath());
}

QFormOptions::~QFormOptions()
{
    delete ui;
}

void QFormOptions::on_pushButton_clicked()
{
    QString curPath=QDir::currentPath();//获取系统当前目录
    QString dlgTitle=tr("设置配置文件的默认保存路径"); //对话框标题
    QString aFilePathName=QFileDialog::getExistingDirectory(this,dlgTitle,curPath);
    if (!aFilePathName.isEmpty())
        ui->lineEdit_Path->setText(aFilePathName);
}

