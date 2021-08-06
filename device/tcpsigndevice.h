#ifndef TCPSIGNDEVICE_H
#define TCPSIGNDEVICE_H

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QTcpSocket>
#include <QTcpServer>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QQueue>
#include "protocol/protocol.h"
#include "config.h"


//回调函数类型
//device：TcpDevice*类型或者QTcpSocket*类型
//      当TcpDevice*为空时，则传QTcpSocket*对象
#define callback_t      std::function<void (void* device, quint8 addr, quint8 type, const QByteArray &data) >




class TcpSignDevice : public QObject
{
    Q_OBJECT

public://初始化
    explicit TcpSignDevice(QObject *parent = nullptr) : QObject(parent){callbackInit();};
    TcpSignDevice(QTcpSocket *tcp, QString id, QString name, quint8 staBytes = 0x08, QObject *parent = nullptr) : QObject(parent),
//        tcp(tcp),
//        info(info),
        id(id),
        name(name),
        stabyte(staBytes)
    {
        if(tcp != nullptr)
        {
            this->ip = tcp->peerAddress().toString();
            this->port = tcp->peerPort();
            this->info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
        }
        q_tcp.clear();
        q_tcp.enqueue(tcp);
        callbackInit();
    };
    TcpSignDevice(QTcpSocket *tcp, QString id, QString name, bool offline = true, bool voice = false, bool flash = false, bool alert = false, QObject *parent = nullptr) : QObject(parent),
//        tcp(tcp),
//        info(info),
        id(id),
        name(name),
        alert(alert),
        flash(flash),
        voice(voice),
        offline(offline)
    {
        if(tcp != nullptr)
        {
            this->ip = tcp->peerAddress().toString();
            this->port = tcp->peerPort();
            this->info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
        }
        q_tcp.clear();
        q_tcp.enqueue(tcp);
        callbackInit();
    };
    ~TcpSignDevice(){};


    //阻塞式发送
    //注意：该函数中的func参数仅限临时使用，所以该函数仅限于发送并接收响应一次。
    //      如需要主动接收下位机信号请调用 registerCallback 或 sendMessageDaemon
    //返回值：
    //      -1：输入无效
    //      0：超时
    //      1：正常
    int sendMessage(const QString& id, quint8 addr, quint8 type, const QByteArray &data = QByteArray(), callback_t func = nullptr, int timeover = REC_TIMEOUT);
    //推荐用上面重载的api
    //仅限于获取到id之后使用该函数
    int sendMessage(quint8 addr, quint8 type, const QByteArray &data = QByteArray(), callback_t func = nullptr, int timeover = REC_TIMEOUT)
    {
        return sendMessage(this->id, addr, type, data, func, timeover);
    };
    //仅限于获取到id之后使用该函数
    int sendFile(const QString& filepath);


public://全局表
    //序列化
    const QString serialization() const;
    void serialization(QJsonObject& json) const;
    //反序列化
    void deserialization(const QString& str);
    void deserialization(const QJsonObject& json);

    //初始化列表
    //需要一开始就调用，获得全局唯一的设备列表
    static void Init();
    //保存警示表
    static bool save();

    static int getDevices(){return DeviceTable.size();};
    static int isEmpty(){return DeviceTable.isEmpty();};
    static bool contains(const QString& _id){return DeviceTable.contains(_id);};
    static TcpSignDevice* find(const QString& _id)
    {
        if(contains(_id))
            return DeviceTable[_id];
        return nullptr;
    };


    static QHash<QString, TcpSignDevice*> DeviceTable;//设备列表 //ID ==> TcpDevice*
    //key  <--> val
    //type <--> (addr, data)  data:有效数据部分，不包括帧头、帧尾、校验位、类型、数据长度，纯粹的数据
    static QHash<quint8, callback_t> CallbackTable;//全局唯一，命令表，用于注册tcp发送后的回调函数

signals://信号
    void received(void);

private://回调函数
    void callbackInit();
    static void cmd_default_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data);
    static void cmd_22_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data);
    static void cmd_72_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data);
    static void cmd_79_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data);
    static void cmd_01_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data);


public://设备属性
    //tcp队列 最新的一直放在最后 可以通过 last() 获取最后一个（last需要确保队列非空）
    QQueue<QTcpSocket *> q_tcp;
    QString info;
    QString ip;
    int port;
    QString id;//编号
    QString name;//标识牌名称
    QString location;//安装位置
//    QString sign;//标示语
    QString signid;//标示语编号
    QString imageidx;//图片编号
    QList<QString> images;//目前所拥有的图片
    quint8 light;//显示亮度
    quint8 vol;//语音音量
    quint8 delay;//提示延时
    quint8 color;//显示颜色

    bool init = false;//初始化状态

    QStandardItem* item = nullptr;//表格中的id一栏的指针
    QString groupname = QString("未命名分组");//组名

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
        quint8 stafault = 0x00;
    };

};

#endif // TCPSIGNDEVICE_H
