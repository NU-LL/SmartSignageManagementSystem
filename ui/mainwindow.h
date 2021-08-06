#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include "device/tcpserver.h"
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

    static void showMessageBox(QMessageBox::Icon icon, QString title, QString text, int ms = 0, bool isBlock = false);


public slots:
    void recMessage(int type, void* message = nullptr);//消息接收槽函数
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
