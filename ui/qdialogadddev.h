#ifndef QDIALOGADDDEV_H
#define QDIALOGADDDEV_H

#include <QDialog>
#include "device/tcpserver.h"
#include "device/tcpsigndevice.h"

//#include "ui/qformmain.h"

namespace Ui {
class QDialogAddDev;
}





class Group: public QObject
{
    Q_OBJECT
public:
    Group(QObject *parent = nullptr):QObject(parent){};
//    Groups(Groups&){};//拷贝构造 允许拷贝
    ~Group(){};




    //通过id找对象
    static bool findGroupName(const QString& groupname)
    {
        return GroupNameSet.contains(groupname);
    };
    static const QList<QString> getGroupNameList(){return GroupNameSet.values();};
    static void addGroup(const QString& groupname){GroupNameSet.insert(groupname);};
    //失败时返回SignDevice的id
    static TcpSignDevice* delGroup(const QString& groupname)
    {
        //确保当前没有正在使用的signdev
        foreach(TcpSignDevice* dev, TcpSignDevice::DeviceTable)
            if(dev->groupname == groupname)
                return dev;
        GroupNameSet.remove(groupname);
        return nullptr;
    };


    //初始化列表
    //需要一开始就调用，获得全局唯一的组表
    static void Init();
    //保存配置
    static bool save();
private:

    //组表
    static QSet<QString> GroupNameSet;
};




class QDialogAddDev : public QDialog
{
    Q_OBJECT

public:
    explicit QDialogAddDev(QWidget *parent = nullptr);
    ~QDialogAddDev();

    const QString getName();
    const QString getDeviceIp();
    const QString getDeviceId();
    const QString getSignText();
    const QString getSignIdx();
    const QString getWarnSignText();
    const QString getWarnSignIdx();
    const QString getGroupName();

private slots:
    void on_comboBox_Device_currentTextChanged(const QString &arg1);

    void on_pushButton_AddGroup_clicked();

    void on_pushButton_DelGroup_clicked();

private:
    Ui::QDialogAddDev *ui;
};

#endif // QDIALOGADDDEV_H
