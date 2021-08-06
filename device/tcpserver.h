#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QTcpServer>
#include "protocol/protocol.h"
#include "config.h"

//#include "device/tcpsigndevice.h"


//#define QUESTION        (0)
//#define INFORMATION     (1)
//#define WARNING         (2)
//#define CRITICAL        (3)


//#define MESSAGE_BOX                 (0)
#define MESSAGE_NEWCONNECTION       (1)
//#define MESSAGE_DISCONNECTION       (2)
#define MESSAGE_DISCONNECTION_INFO  (3)
#define MESSAGE_ADDDEVICE           (4)
//#define MESSAGE_ADDDEVICE_STR       (5)
#define MESSAGE_CHANGE_STATUS       (6)



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

    bool startServer(quint16 port);                                                 //启动服务器
    void stopServer();                                                              //停止服务器
    bool serverStatus(){return m_server.isListening();};                            //服务器状态
    int getClients(){return m_server.findChildren<QTcpSocket *>().size();};         //获得客户端数量
    bool isClientEmpty(){return m_server.findChildren<QTcpSocket *>().isEmpty();};  //是否有客户端连接


    //hex字符串 转 hex
    //"A1 A2" ==> [A1 A2]
    static QByteArray hexStringToByteArray(const QString hexstring);
    //hex 转 hex字符串
    //[A1 A2] ==> "A1 A2"
    static QString byteArrayToHexString(const QByteArray &cba);



//    qint64 send(const char *data, qint64 maxSize, const QString& objectName = QString());
    qint64 send(const QByteArray &byteArray, const QString& objectName = QString());


    //非阻塞发送
    int sendMessage_noblock(quint8 addr, quint8 type, const QByteArray& data, const QString& objectName);

signals:
    //仅仅用于向界面发送必要的消息
    //type:消息类型
    //message:消息内容 特定的消息类型才会用到
    void message(int type, void* message = nullptr);//向界面发送信息，通过type区分不同信息
    void recData(QTcpSocket *tcp, const QByteArray& data);//接收到消息后，会通过改信号将消息发送出去

private:
    explicit TcpServer(QObject *parent = nullptr);

    //初始化客户端信号槽
    void initServerSignals(){connect(&m_server, &QTcpServer::newConnection, this, &TcpServer::onServerNewConnection);};

    //周期定时器 心跳状态包
    QTimer *HeartbeatTimer = nullptr;
    quint16 timerInterval = 10000;//ms 每个节点之间的时间间隔

private slots:
//    void timeout(bool isinit = false);
    void timeout();
//    void timeout(bool isinit);
//    void timeout(){timeout(false);};
    void onServerNewConnection();
    void onServerConnected();
    void onServerDisconnected_before();
    void onServerDisconnected();
    void onServerDataReady();
    void onServerBytesWritten(qint64 bytes);

signals:

};

#endif // TCPSERVER_H
