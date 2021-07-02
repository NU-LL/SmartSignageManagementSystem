#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tcpserver.h"
#include "config.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE





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



public slots:
    void recMessage(int level, QString title, QString text, int message_id = MESSAGE_BOX, void* message = nullptr);//消息接收槽函数
    void recData(QTcpSocket* tcp, const QByteArray& data);//数据接收槽函数

private slots:

    void on_actionDebug_triggered();

    void on_actionAbout_triggered();

    void on_actionOptions_triggered();

    void on_actionMain_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
