#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tcpserver.h"
#include "config.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


//回调函数类型
//typedef void(*callback_t)(quint8 addr, const QByteArray &data);
#define callback_t      std::function<void (quint8 addr, const QByteArray &data)>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //hex字符串 转 hex
    //"A1 A2" ==> [A1 A2]
    QByteArray hexStringToByteArray(const QString hexstring);
    //hex 转 hex字符串
    //[A1 A2] ==> "A1 A2"
    QString byteArrayToHexString(const QByteArray &cba);

    void registerCallback(quint8 type, callback_t func = [](quint8, const QByteArray &){});
    void disregisterCallback(quint8 type);

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

    QMap<QString, int> Ip2IdTable;//IP ==> ID
    QMap<int, QString> Id2IpTable;//ID ==> Ip

public slots:
    void recMessage(int level, QString title, QString text, int message_id = MESSAGE_BOX);//消息接收槽函数
    void recData(QTcpSocket* tcp, const QByteArray& data);//数据接收槽函数

private slots:

    void on_actionDebug_triggered();

    void on_actionAbout_triggered();

    void on_actionOptions_triggered();

    void on_actionMain_triggered();

private:
    Ui::MainWindow *ui;

    //key  <--> val
    //type <--> (addr, data)  data:有效数据部分，不包括帧头、帧尾、校验位、类型、数据长度，纯粹的数据
    QMap<quint8, callback_t> CallbackTable;//全局唯一，命令表，用于注册tcp发送后的回调函数
};
#endif // MAINWINDOW_H
