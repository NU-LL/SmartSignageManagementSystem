#ifndef QFORMDEBUG_H
#define QFORMDEBUG_H

#include <QWidget>
#include "tcpserver.h"

namespace Ui {
class QFormDebug;
}

class QFormDebug : public QWidget
{
    Q_OBJECT

public:
    //单例模式
    //注意：该函数线程不安全（C++构造函数本身就是线程不安全）
//    static QFormDebug* instance()
//    {
//        static QFormDebug *formDebug = new QFormDebug;
//        return formDebug;
//    }
    explicit QFormDebug(QWidget *parent = nullptr);
    ~QFormDebug();


    QString hexDump(const QByteArray &data, bool ishtml = false);

    QString getObjectName();//获得现在的对象名称

public slots:
    void recMessage(int level, QString title, QString text, int message_id = MESSAGE_BOX);//消息接收槽函数
    void recData(QTcpSocket* tcp, const QByteArray& data);//数据接收槽函数

private slots:
    void on_pushButton_StartServer_clicked();

    void on_pushButton_Send_clicked();

    void on_pushButton_CustomSend_clicked();

    void on_pushButton_DebugCmd_clicked();

private:


    Ui::QFormDebug *ui;
};

#endif // QFORMDEBUG_H
