#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>


#define QUESTION        (0)
#define INFORMATION     (1)
#define WARNING         (2)
#define CRITICAL        (3)


#define MESSAGE_BOX                 (0)
#define MESSAGE_NEWCONNECTION       (1)
#define MESSAGE_DISCONNECTION       (2)


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

    bool startServer(quint16 port);                                     //启动服务器
    void stopServer();                                                  //停止服务器
    bool serverStatus(){return m_server.isListening();};                //服务器状态
    qint64 send(const char *data, qint64 maxSize, QString objectName = QString());
    qint64 send(const QByteArray &byteArray, QString objectName = QString());

signals:
    //level：QMessageBox级别
    //      3,2,1,0<==>critical,warning,information,question
    //title：QMessageBox标题
    //text：QMessageBox内容
    //message_id:消息类型
    //
    void message(int level, QString title, QString text, int message_id = MESSAGE_BOX);//过程中的一些提醒，一般用于 QMessageBox 打印
    void recData(QTcpSocket *tcp, const QByteArray& data);//接收到消息后，会通过改信号将消息发送出去

private:
    explicit TcpServer(QObject *parent = nullptr);

    void initServerSignals();   //初始化客户端信号槽


private slots:
    void onServerNewConnection();
    void onServerConnected();
    void onServerDisconnected();
    void onServerDataReady();
    void onServerBytesWritten(qint64 bytes);

signals:

};

#endif // TCPSERVER_H
