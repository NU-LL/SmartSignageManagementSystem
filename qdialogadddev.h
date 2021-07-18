#ifndef QDIALOGADDDEV_H
#define QDIALOGADDDEV_H

#include <QDialog>
#include "tcpserver.h"

namespace Ui {
class QDialogAddDev;
}





class Group: public QObject
{
    Q_OBJECT
public:
    //初始化列表
    //需要一开始就调用，获得全局唯一的组表
    static void Init();

    Group(QObject *parent = nullptr):QObject(parent){};
//    Groups(Groups&){};//拷贝构造 允许拷贝
    ~Group(){};

    static const QStringList& getGroupNameList(){return GroupNameList;};
    static void addGroup(const QString& name){GroupNameList.append(name);};
    static bool delGroup(const QString& name){return GroupNameList.removeOne(name);};//对于列表来说 仅仅只移除第一个相等的元素

    //保存配置
    static bool saveGroup();
private:

    //组表
    static QStringList GroupNameList;
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
    const QString getGroupName();

private slots:
    void on_comboBox_Device_currentTextChanged(const QString &arg1);

    void on_pushButton_AddGroup_clicked();

private:
//    bool eventFilter(QObject *obj, QEvent *e);


private:
    Ui::QDialogAddDev *ui;
};

#endif // QDIALOGADDDEV_H
