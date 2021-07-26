#ifndef QFORMMAIN_H
#define QFORMMAIN_H

#include <QWidget>
#include <QLabel>
#include <QBitArray>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QCloseEvent>
#include <QItemDelegate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include "tcpserver.h"

#include <QMessageBox>

#include "qformsigntable.h"

namespace Ui {
class QFormMain;
}



class SignDevice : public QObject
{
    Q_OBJECT
public:
    explicit SignDevice(QObject *parent = nullptr) : QObject(parent){};
    SignDevice(const QString& id, const QString& groupname, const QString& name, const QString& signid, QObject *parent = nullptr)
        :QObject(parent), id(id), groupname(groupname), name(name), signid(signid) {};
//        :TcpDevice("", id, name, 0x08, parent), groupname(groupname), signid(signid) {};
//    SignDevice(SignDevice&){};//拷贝构造 允许拷贝

    //对 TcpDevice 的转换
    SignDevice(TcpDevice& tcpdev)
        :senderName(tcpdev.info), id(tcpdev.id), groupname(QString("未命名分组")), name(tcpdev.name), signid(tcpdev.signid)
        , stabyte(tcpdev.stabyte), stafault(tcpdev.stafault) {};//拷贝构造
    SignDevice(TcpDevice* tcpdev)
        :senderName(tcpdev->info), id(tcpdev->id), groupname(QString("未命名分组")), name(tcpdev->name), signid(tcpdev->signid)
        , stabyte(tcpdev->stabyte), stafault(tcpdev->stafault){};//拷贝构造
    SignDevice& operator=(const TcpDevice& tcpdev)
    {
        this->senderName = tcpdev.info;
        this->id = tcpdev.id;
        this->name = tcpdev.name;
        this->signid = tcpdev.signid;
        this->stabyte = tcpdev.stabyte;
        this->stafault = tcpdev.stafault;
        return *this;
    };

    ~SignDevice(){};

public:
    //各种属性
    QString senderName;//用于发送数据时确定对象用


    QString id;//编号
    QString groupname = QString("未命名分组");//组名
    QString name;//标识牌名称
    QString signid;//标示语id

    QStandardItem* item = nullptr;//表格中的id一栏的指针

    //状态字
    union{
        struct{
            quint8 alert:1;//最低位
            quint8 flash:1;
            quint8 voice:1;
            quint8 offline:1;//最高位
            quint8 undefined:4;
        };
        quint8 stabyte = 0x08;//状态字 默认离线
    };
    //异常状态
    union{
        struct{
            quint8 people_approach:1;//人员靠近 //最低位
            quint8 Power:1;//电源故障
            quint8 controller:1;//控制器故障
            quint8 power_off:1;//交流电断电
            quint8 low_battery:1;//电池电量过低
            quint8 manual_configuration:1;//红外遥控手动配置中 //最高位
            quint8 undefined:2;
        }fault;
        quint8 stafault = 0x00;//默认无警报
    };

public:
    //序列化
    const QString serialization() const;
    void serialization(QJsonObject& json) const;
    //反序列化
    void deserialization(const QString& str);
    void deserialization(const QJsonObject& json);

    //获得标示语信息
    const QString getSignText()
    {
        Sign *sign = Sign::findSign(this->signid);
        if(sign == nullptr)
            return QString("");
        return sign->text;
    };
    const QIcon getSignIcon()
    {
        Sign *sign = Sign::findSign(this->signid);
        if(sign == nullptr)
            return QIcon("");
        return sign->getIcon();
    }
    const QColor getSignColor()
    {
        Sign *sign = Sign::findSign(this->signid);
        if(sign == nullptr)
            return QColor(0, 0, 0);//默认黑色
        return sign->getColor();
    }



public:
    //初始化列表
    //需要一开始就调用，获得全局唯一的设备列表
    static void Init();
    //保存警示表
    static bool save();

    //通过id找对象
    static SignDevice* findSignDev(const QString& id)
    {
//        SignDeviceTable.value(id);//如果键不存在，则利用值类型的默认构造函数，将返回一个默认值，同时不会创建新的项
        auto iter = SignDeviceTable.find(id);
        if(iter != SignDeviceTable.end())
            return iter.value();
        return nullptr;
    };
    //注册警示语到警示语表中
    static void registerSignDev(const QString& id, SignDevice* sign){SignDeviceTable[id] = sign;};
    static void unregisterSignDev(const QString& id){delete SignDeviceTable[id];SignDeviceTable.remove(id);};
    //清空整个表
    static void clearSignDevTable()
    {
        foreach(SignDevice* it, SignDeviceTable)
            delete it;
        SignDeviceTable.clear();
    };
    //获取整个表，仅仅只有可读权限
    //注意，由于表中存的的对象指针，仍可以通过该指针修改设备（这里仅仅是不能修改这个map）
    static const QHash<QString, SignDevice*>& getSignDevTable(){return SignDeviceTable;};

private:

    //设备表
//    QHash
    static QHash<QString, SignDevice*> SignDeviceTable;
};








////标示语代理
//class signDelegate: public QItemDelegate
//{
//    Q_OBJECT
//public:
//    signDelegate(QMap<int, QString>& SignTable, QObject *parent = 0):SignTable(SignTable){};

//public:
//    //创建控件
//    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
//    //设置控件显示的数据 将Model中的数据更新到Delegate中
//    void setEditorData(QWidget *editor, const QModelIndex &index) const;
//    //将Delegate中的数据更新到Model中
//    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
//    //更新控件区的显示
//    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const{
//        editor->setGeometry(option.rect);
//    };
//private:
//    QMap<int, QString>& SignTable;

//private slots:
//    void currentTextChanged(const QString &str);
//    void currentIndexChanged(int idx);
//};
//按键代理
class buttonDelegate: public QItemDelegate
{
    Q_OBJECT
public:
    buttonDelegate(QObject *parent = 0){};

public:
    //创建控件
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    //设置控件显示的数据 将Model中的数据更新到Delegate中
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    //将Delegate中的数据更新到Model中
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    //更新控件区的显示
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const{
        editor->setGeometry(option.rect);
    };
};







class QFormMain : public QWidget
{
    Q_OBJECT

private:
    QStandardItemModel * theModel; //数据模型
    QItemSelectionModel *theSelection; //选择模型

    //事件截获
    bool eventFilter(QObject *watched, QEvent *event);
    //关闭窗口信号
    void closeEvent(QCloseEvent *event);
    //修改某一项
    bool modifyCell(int row, int column, const QString &text, const QIcon &icon = QIcon()){
        QStandardItem* aItem = theModel->item(row, column);
        if(aItem == nullptr)
        {
            QMessageBox::warning(this, "警告", QString("修改(%1, %2)处失败，超出范围").arg(row).arg(column));
            return false;
        }
        if(aItem->text() == text)
            return false;
        else
        {
            aItem->setText(text);
            if(!icon.isNull())
                aItem->setIcon(icon);
        }
        return true;
    };

public:
//    //单例模式
//    //注意：该函数线程不安全（C++构造函数本身就是线程不安全）
//    static QFormMain* instance()
//    {
//        static QFormMain *formSignTable = new QFormMain;
//        return formSignTable;
//    }
    explicit QFormMain(QWidget *parent = nullptr);
    ~QFormMain();

    //更新 SignDevice 中的数据到表格中
    //返回值：哪一位置1则表示该列更新
    QBitArray refreshSignDev(SignDevice* signdev);

    void addDevice(SignDevice* signdev)
    {
        signdev->item = addLine(signdev);
//        signdev->item = addLine(signdev->id, signdev->groupname, signdev->name, signdev->signid, signdev->voice==1, signdev->flash==1, signdev->alert==1);
        SignDevice::registerSignDev(signdev->id, signdev);
    };
    void delDevice(int row)
    {
        QString id = theModel->item(row,0)->text();
        SignDevice::unregisterSignDev(id);
        delLine(row);
    };



    //设置状态
//    void setStatus(int row, quint8 staBytes);



private:
    //设置分组下拉框
    void setGroupCombox(QComboBox *combobox);
    //设置警示语下拉框
    void setSignCombox(QComboBox *combobox);
    //设置开关选项下拉框
    void setSwitchCombox(QComboBox *combobox);

    //添加一行
    //返回第一栏（id）的指针（插入失败返回空）
    QStandardItem* addLine(const SignDevice* signdev);
    //（已经废弃）
    QStandardItem* addLine(const QString& id, const QString& groupname, const QString& name, const QString& signid, bool voice=false, bool flash=false, bool alert=false);
    //删除一行
    void delLine(int row){theModel->removeRow(row);};



public slots:
    void recMessage(int level, QString title, QString text, int message_id = MESSAGE_BOX, void* message = nullptr);//消息接收槽函数

private slots:
    void on_pushButton_AddDevice_clicked();

    void on_pushButton_DelDevice_clicked();

    void on_pushButton_SaveTable_clicked();

    void on_pushButton_SignTable_clicked();

    void on_tableView_clicked(const QModelIndex &index);

    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    Ui::QFormMain *ui;
};

#endif // QFORMMAIN_H
