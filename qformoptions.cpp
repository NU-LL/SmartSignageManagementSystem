#include "qformoptions.h"
#include "ui_qformoptions.h"
#include "config.h"

#include <QDir>
#include <QFileDialog>



QString QFormOptions::ConfigFilePath = QDir(DEFAULT_PROFILE_PATH).absolutePath();
QString QFormOptions::IconPath = QDir(DEFAULT_ICON_PATH).absolutePath();

QFormOptions::QFormOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormOptions)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除

//    ui->lineEdit_Path->setText(QDir::currentPath());
//    ui->lineEdit_ConfigFilePath->setText(QDir(DEFAULT_PROFILE_PATH).absolutePath());
    if(!QDir(ConfigFilePath).exists())
        QDir().mkpath(ConfigFilePath);
    if(!QDir(IconPath).exists())
        QDir().mkpath(IconPath);
    ui->lineEdit_ConfigFilePath->setText(ConfigFilePath);
    ui->lineEdit_IconPath->setText(IconPath);
}

QFormOptions::~QFormOptions()
{
    delete ui;
}

void QFormOptions::on_pushButton_ConfigFile_clicked()
{
    QString aFilePathName=QFileDialog::getExistingDirectory(this, tr("设置配置文件的默认保存路径"), ConfigFilePath);
    if (!aFilePathName.isEmpty())
    {
        ui->lineEdit_ConfigFilePath->setText(aFilePathName);
        ConfigFilePath = aFilePathName;
    }
}


void QFormOptions::on_pushButton_Icon_clicked()
{
    QString aFilePathName=QFileDialog::getExistingDirectory(this, tr("设置图标文件的默认保存路径"), IconPath);
    if (!aFilePathName.isEmpty())
    {
        ui->lineEdit_IconPath->setText(aFilePathName);
        IconPath = aFilePathName;
    }
}

