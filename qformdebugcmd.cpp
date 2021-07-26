#include "qformdebugcmd.h"
#include "ui_qformdebugcmd.h"

#include <qdebug.h>
#include <QMessageBox>

#include "tcpserver.h"
//#include "mainwindow.h"
#include "qformdebug.h"
#include "protocol.h"

QFormDebugCmd::QFormDebugCmd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormDebugCmd)
{
    ui->setupUi(this);
}

QFormDebugCmd::~QFormDebugCmd()
{
    delete ui;
}

//设置声音
void QFormDebugCmd::on_pushButton_set_Voice_clicked()
{
    //parentWidget：当前窗口的父窗口，反复调用均返回同一个值（如：parentWidget()->parentWidget()）
    //window：当前控件所在的窗口
    //nativeParentWidget：该控件的顶层窗口
    QFormDebug *ptrDebug = (QFormDebug*)parentWidget();//获取上一级父窗口 注意：反复调用 parentWidget 仍是 QFormDebug
    QString objName = ptrDebug->getObjectName();
    if(objName.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请先连接设备"));
        return ;
    }
//    nativeParentWidget()->setWindowTitle("New Window Title");//用该函数可以测试是那个窗口
//    MainWindow *ptrMain = (MainWindow*)nativeParentWidget();//获得顶层窗口

    QByteArray data;
    data += ui->horizontalSlider_Voice->value() + '0';
    //发送消息 并 注册回调函数
    TcpServer& server = TcpServer::getHandle();
//    server.sendMessage(02, data, objName);
    //发送完关闭即销毁 防止该界面关闭后仍回调
//    server.disregisterCallback(02);
}

//设置亮度
void QFormDebugCmd::on_pushButton_set_Lightness_clicked()
{
    QFormDebug *ptrDebug = (QFormDebug*)parentWidget();//获取上一级父窗口 注意：反复调用 parentWidget 仍是 QFormDebug
    QString objName = ptrDebug->getObjectName();
    if(objName.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请先连接设备"));
        return ;
    }
//    MainWindow *ptrMain = (MainWindow*)nativeParentWidget();//获得顶层窗口

    QByteArray data;
    data += QString("%1").arg(ui->horizontalSlider_Lightness->value(), 2, 10, QLatin1Char('0')).toLocal8Bit();
    //发送消息 并 注册回调函数
    TcpServer& server = TcpServer::getHandle();
//    server.sendMessage(05, data, objName);
    //发送完关闭即销毁 防止该界面关闭后仍回调
//    server.disregisterCallback(05);
}

