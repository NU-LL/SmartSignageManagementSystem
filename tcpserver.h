#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QTcpServer>
#include "protocol.h"
#include "config.h"


#define QUESTION        (0)
#define INFORMATION     (1)
#define WARNING         (2)
#define CRITICAL        (3)


#define MESSAGE_BOX                 (0)
#define MESSAGE_NEWCONNECTION       (1)
#define MESSAGE_DISCONNECTION       (2)
#define MESSAGE_ADDDEVICE           (3)
#define MESSAGE_CHANGE_STATUS       (4)

//回调函数类型
//typedef void(*callback_t)(quint8 addr, const QByteArray &data);
#define callback_t      std::function<void (TcpDevice* device, quint8 addr, const QByteArray &data)>


class TcpDevice : public QObject
{
    Q_OBJECT
public:
    explicit TcpDevice(QObject *parent = nullptr) : QObject(parent){};
    TcpDevice(QString info, QString id, QString name, quint8 staBytes = 0, QObject *parent = nullptr) : QObject(parent),
        info(info),
        id(id),
        name(name),
        stabyte(staBytes)
    {};
    TcpDevice(QString info, QString id, QString name, bool offline = false, bool voice = false, bool flash = false, bool alert = false, QObject *parent = nullptr) : QObject(parent),
        info(info),
        id(id),
        name(name),
        offline(offline),
        voice(voice),
        flash(flash),
        alert(alert)
    {};
    ~TcpDevice(){};

public:
    QString info;
    QString ip;
    int port;
    QString id;//编号
    QString name;//名称
    QString location;//安装位置
    QString sign;//标示语
    int signidx;//标示语编号
    int imageidx;//图片编号
    quint8 light;//显示亮度
    quint8 vol;//语音音量
    quint8 delay;//提示延时

    //状态字
    union{
        struct{
            quint8 alert:1;//最低位
            quint8 flash:1;
            quint8 voice:1;
            quint8 offline:1;//最高位
            quint8 undefined:4;
        };
        quint8 stabyte;//状态字
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
        quint8 stafault;
    };
private:
};




class TcpServer : public QObject
{
    Q_OBJECT
public:
    //单例模式
    //注意：该函数线程不安全（C++构造函数本身就是线程不安全）
    static TcpServer& getHandle()
    {
        static TcpServer tcpserver;
        return tcpserver;
    }
    ~TcpServer();

    QTcpServer m_server;

    //设置和获取（心跳包）定时器间隔时间
    void setTimerInterval(quint16 interval){timerInterval = interval;};
    quint16 getTimerInterval(void){return timerInterval;};

    bool startServer(quint16 port);                                     //启动服务器
    void stopServer();                                                  //停止服务器
    bool serverStatus(){return m_server.isListening();};                //服务器状态


    //hex字符串 转 hex
    //"A1 A2" ==> [A1 A2]
    QByteArray hexStringToByteArray(const QString hexstring);
    //hex 转 hex字符串
    //[A1 A2] ==> "A1 A2"
    QString byteArrayToHexString(const QByteArray &cba);



    qint64 send(const char *data, qint64 maxSize, QString objectName = QString());
    qint64 send(const QByteArray &byteArray, QString objectName = QString());




    bool registerCallback(quint8 type, callback_t func = [](TcpDevice*, quint8, const QByteArray &){});
    bool disregisterCallback(quint8 type);

    //非阻塞发送
    int sendMessage_noblock(quint8 addr, quint8 type, const QByteArray &data, QString objectName);

    //阻塞式发送 不可重入
    //返回值：
    //0：正常
    //-1：发送失败
    //-2：超时
    //-3：重入
    int sendMessage(quint8 addr, quint8 type, const QByteArray &data, QString objectName, callback_t func = nullptr, int timeover = REC_TIMEOUT);
    int sendMessage(quint8 type, const QByteArray &data, QString objectName, callback_t func = nullptr, int timeover = REC_TIMEOUT)
                    {return sendMessage(0, type, data, objectName, func, timeover);};

    //回调完成标志，在 sendMessage 中，如果有回调函数，则完成时一定要设置改标志位
    bool callback_completion_flag = false;

    //以下的map小心使用 以防插入多余元素
    QMap<QString, QString> Ip2IdTable;//IP:port ==> ID
    QMap<QString, QString> Id2IpTable;//ID ==> IP:port
    QMap<QString, TcpDevice*> DeviceTab;//设备列表 //ID ==> TcpDevice*
signals:
    //level：QMessageBox级别
    //      3,2,1,0<==>critical,warning,information,question
    //title：QMessageBox标题
    //text：QMessageBox内容
    //message_id:消息类型
    //message:消息内容 特定的消息类型才会用到
    //
    void message(int level, QString title, QString text, int message_id = MESSAGE_BOX, void* message = nullptr);//过程中的一些提醒，一般用于 QMessageBox 打印
    void recData(QTcpSocket *tcp, const QByteArray& data);//接收到消息后，会通过改信号将消息发送出去

private:
    explicit TcpServer(QObject *parent = nullptr);

    //初始化客户端信号槽
    void initServerSignals(){connect(&m_server, &QTcpServer::newConnection, this, &TcpServer::onServerNewConnection);};
    //绑定映射表
    void bindTable(QString info, QString id)
    {
        Ip2IdTable[info] = id;
        Id2IpTable[id] = info;
    };
    //解绑
    void unbindTable(QString info, QString id)
    {
        Ip2IdTable.remove(info);
        Id2IpTable.remove(id);
    };

    //key  <--> val
    //type <--> (addr, data)  data:有效数据部分，不包括帧头、帧尾、校验位、类型、数据长度，纯粹的数据
    QMap<quint8, callback_t> CallbackTable;//全局唯一，命令表，用于注册tcp发送后的回调函数

    //周期定时器 心跳状态包
    QTimer *HeartbeatTimer = nullptr;
    quint16 timerInterval = 5000;//ms

private slots:
    void timeout();
    void onServerNewConnection();
    void onServerConnected();
    void onServerDisconnected();
    void onServerDataReady();
    void onServerBytesWritten(qint64 bytes);

signals:

};

#endif // TCPSERVER_H
