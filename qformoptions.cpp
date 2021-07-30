#include "qformoptions.h"
#include "ui_qformoptions.h"
#include "config.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QFileDialog>
#include <QDebug>


QString QFormOptions::ConfigFilePath = QDir(DEFAULT_PROFILE_PATH).absolutePath();
QString QFormOptions::IconPath = QDir(DEFAULT_ICON_PATH).absolutePath();
bool QFormOptions::defaultStartServer = true;



//初始化列表
//需要一开始就调用，获得全局唯一的组表
void QFormOptions::Init()
{
    static bool initflag = false;
    if(initflag)//确保只初始化一次
        return ;

    //json文件存储
    QFile loadFile(QDir(QFormOptions::ConfigFilePath).absoluteFilePath("options.json"));
    if(!loadFile.exists())
    {
        loadFile.open(QIODevice::WriteOnly);
        loadFile.close();
        save();//直接保存默认数据
    }
    if (!loadFile.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open load file : options.json.");
        return;
    }
    QByteArray loadData = loadFile.readAll();
    loadFile.close();
    QJsonDocument loadDoc(QJsonDocument::fromJson(loadData));
//    QJsonDocument loadDoc(QJsonDocument::fromBinaryData(saveData));

    QJsonObject json = loadDoc.object();
    ConfigFilePath = json["ConfigFilePath"].toString();
    IconPath = json["IconPath"].toString();
    defaultStartServer = json["defaultStartServer"].toBool();


    initflag = true;
}

//保存配置
bool QFormOptions::save()
{
    //json文件存储（注意，会清空）
    QFile saveFile(QDir(QFormOptions::ConfigFilePath).absoluteFilePath("options.json"));
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        qWarning("Couldn't open save file : options.json.");
        return false;
    }
    //遍历整个警示语表 存入json
    QJsonObject jsonObj;
    jsonObj["ConfigFilePath"] = ConfigFilePath;
    jsonObj["IconPath"] = IconPath;
    jsonObj["defaultStartServer"] = defaultStartServer;

    QJsonDocument saveDoc(jsonObj);
    saveFile.write(saveDoc.toJson());
//    saveFile.write(saveDoc.toBinaryData());
    saveFile.close();
    return true;
}



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
        save();
    }
}


void QFormOptions::on_pushButton_Icon_clicked()
{
    QString aFilePathName=QFileDialog::getExistingDirectory(this, tr("设置图标文件的默认保存路径"), IconPath);
    if (!aFilePathName.isEmpty())
    {
        ui->lineEdit_IconPath->setText(aFilePathName);
        IconPath = aFilePathName;
        save();
    }
}


void QFormOptions::on_checkBox_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked)//勾选
    {
        defaultStartServer = true;
    }else
        defaultStartServer = false;
    save();
}

