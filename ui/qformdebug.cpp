#include "qformdebug.h"
#include "ui_qformdebug.h"
#include "config.h"

#include <qdebug.h>
#include <QMessageBox>


#include "protocol/protocol.h"
#include "ui/mainwindow.h"
#include "qformdebugcmd.h"

#include "device/tcpserver.h"
#include "device/tcpsigndevice.h"

QFormDebug::QFormDebug(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormDebug)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除

    //tcp server
    TcpServer& server = TcpServer::getHandle();
    if(server.serverStatus())
    {
        //服务已经启动
        ui->label_ServerIp->setText(server.m_server.serverAddress().toString());
        ui->lineEdit_Port->setText(QString::number(server.m_server.serverPort()));
        ui->lineEdit_Port->setEnabled(false);
        ui->pushButton_StartServer->setText(tr("停止服务"));
        ui->pushButton_DebugCmd->setEnabled(true);
    }
    //填充 comboBox_DeviceInfo
    QList<QTcpSocket *> m_tcps = server.m_server.findChildren<QTcpSocket *>();
    foreach (QTcpSocket *tcp, m_tcps)
        ui->comboBox_DeviceInfo->addItem(tcp->objectName());

    if(ui->comboBox_DeviceInfo->count() != 0)
    {
        const QString& info = ui->comboBox_DeviceInfo->currentText();
        QList<QTcpSocket *> list = server.m_server.findChildren<QTcpSocket *>(info);
        TcpSignDevice* device = list[0]->property("TcpSignDevice").value<TcpSignDevice*>();
        ui->label_DeviceId->setText(device->id.isEmpty()?"未找到对应元素":device->id);
    }else
        ui->label_DeviceId->setText("请连接设备获取id");

    //连接信号槽
    connect(&server, static_cast<void (TcpServer:: *)(int, void*)>(&TcpServer::message),
            this, static_cast<void (QFormDebug:: *)(int, void*)>(&QFormDebug::recMessage));
    connect(&server, static_cast<void (TcpServer:: *)(QTcpSocket *, const QByteArray& data)>(&TcpServer::recData),
            this, static_cast<void (QFormDebug:: *)(QTcpSocket *, const QByteArray& data)>(&QFormDebug::recData));
}

QFormDebug::~QFormDebug()
{
    //tcp server
    TcpServer& server = TcpServer::getHandle();
    disconnect(&server, static_cast<void (TcpServer:: *)(QTcpSocket *, const QByteArray& data)>(&TcpServer::recData),
               this, static_cast<void (QFormDebug:: *)(QTcpSocket *, const QByteArray& data)>(&QFormDebug::recData));
    disconnect(&server, static_cast<void (TcpServer:: *)(int, void*)>(&TcpServer::message),
               this, static_cast<void (QFormDebug:: *)(int, void*)>(&QFormDebug::recMessage));
    delete ui;
}

//消息接收槽函数
//level：QMessageBox级别
//      3,2,1,0<==>critical,warning,information,question
void QFormDebug::recMessage(int type, void* message)
{
    switch(type)
    {
        case(MESSAGE_NEWCONNECTION):
        {
            //刚刚链接时 没有建立TcpDevice对象 所以只能传info
            //此时label_DeviceId必定设置不了
            const QString info = *(const QString*)message;
//            qDebug() << "添加设备 " << info;
            ui->comboBox_DeviceInfo->addItem(info);
//            TcpServer& server = TcpServer::getHandle();
//            ui->label_DeviceId->setText(server.findId(ip));
            break;
        }
        case(MESSAGE_DISCONNECTION):
        {
            TcpSignDevice* dev = static_cast<TcpSignDevice*>(message);
            int idx = ui->comboBox_DeviceInfo->findText(dev->info);
            ui->comboBox_DeviceInfo->removeItem(idx);//如果idx无效什么也不会做
            break;
        }
    }
}

//消息接收槽函数
void QFormDebug::recData(QTcpSocket *tcp, const QByteArray& data)
{
//    QByteArray buf = tcp->readAll();
//    qDebug() << "[接受来自"+ tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort())+"]:";
//    qDebug() << QString::fromLocal8Bit(buf) << " <==> " << byteArrayToHexString(buf);

    ui->textBrowser_Receive->append("<span style=\" color:#55aa7f;\">["+ tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort())+"]:" + "</span>");
    if(ui->checkBox_HexDisplay->isChecked())
    {
//        MainWindow *ptr = (MainWindow*)parentWidget();//获取父窗口
        TcpServer& server = TcpServer::getHandle();
        ui->textBrowser_Receive->append("<span style=\" color:#55aa7f;\">" + server.byteArrayToHexString(data) + "</span>");
    }
    else
        ui->textBrowser_Receive->append("<span style=\" color:#55aa7f;\">" + QString::fromLocal8Bit(data) + "</span>");

    if(ui->checkBox_AutoDecode->isChecked())
    {
        //自动解码
        static Protocol protocol;
        protocol.process(data);
        if(!protocol.isEmpty())
        {
            //接收到一帧数据
            Frame frame = protocol.getFrame();
            //<html><head/><body><p><span style=" color:#55aa7f;">你好</span></body></html>
            QString str = QString("设备地址：%1 帧类型：%2 数据长度：%3")
                    .arg(frame.addr(), 2, 10, QLatin1Char('0'))
                    .arg(frame.type(), 2, 10, QLatin1Char('0'))
                    .arg(frame.length());
            ui->textBrowser_Receive->append("<span style=\" color:#55aa7f;\">" + str + "</span>");
            ui->textBrowser_Receive->append("<span style=\" color:#55aa7f;\">" + hexDump(frame.getData(), true) + "</span>");

            qDebug() << "hexdump:\n" << hexDump(frame.getData());
        }
    }
}





//hexdump
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
QString QFormDebug::hexDump(const QByteArray &data, bool ishtml)
{
    QString buf;
    int i, j;

    for (i = 0; i < data.length(); i += 16)
    {
        buf += QString("%1: ").arg(i, 8, 16, QLatin1Char('0'));

        for (j = 0; j < 16; j++)
            if (i+j < data.length())
                buf += QString("%1 ").arg(data[i+j], 2, 16, QLatin1Char('0'));
            else
                if(ishtml)
                    buf += QString("&nbsp;&nbsp;&nbsp;");
                else
                    buf += QString("   ");
        buf += QString(" ");

        for (j = 0; j < 16; j++)
            if (i+j < data.length())
                buf += QString(__is_print(data[i+j]) ? data[i+j] : '.');
        if(ishtml)
            buf += "<br/>";
        else
            buf += '\n';
    }
    return buf;
}


//获得现在的对象名称
QString QFormDebug::getObjectName()
{
    return ui->comboBox_DeviceInfo->currentText();
}



//启动服务
void QFormDebug::on_pushButton_StartServer_clicked()
{
    //tcp server
    TcpServer& server = TcpServer::getHandle();
    if(server.serverStatus())
    {
        //服务已经启动，此时需要关闭
        server.stopServer();
        ui->pushButton_StartServer->setText(tr("启动服务"));
        ui->lineEdit_Port->setEnabled(true);
        ui->comboBox_DeviceInfo->clear();
        ui->pushButton_DebugCmd->setEnabled(false);
    }else
    {
        //服务未启动，此时需要打开
        if(server.startServer(ui->lineEdit_Port->text().toInt()))
        {
            ui->label_ServerIp->setText(server.m_server.serverAddress().toString());
            ui->lineEdit_Port->setText(QString::number(server.m_server.serverPort()));
        }else
        {
            QMessageBox::warning(this, tr("警告"), tr("启动服务失败"));
            return;
        }
        ui->pushButton_StartServer->setText(tr("停止服务"));
        ui->lineEdit_Port->setEnabled(false);
        ui->pushButton_DebugCmd->setEnabled(true);
    }
}

//发送
void QFormDebug::on_pushButton_Send_clicked()
{
    if(ui->pushButton_StartServer->text() == QString("启动服务"))
    {
        QMessageBox::information(this, tr("提示"), tr("请先启动服务"));
        return;
    }

    if(ui->comboBox_DeviceInfo->count() == 0)
    {
        QMessageBox::information(this, tr("提示"), tr("请等待设备连接"));
        return;
    }

    //tcp server
    TcpServer& server = TcpServer::getHandle();
    if(ui->checkBox_HexSend->isChecked())
    {
        //hex 发送
//        MainWindow *ptr = (MainWindow*)parentWidget();//获取父窗口
        if(ui->checkBox_AllSend->isChecked())
            server.send(server.hexStringToByteArray(ui->textEdit_Send->toPlainText()));
        else
            server.send(server.hexStringToByteArray(ui->textEdit_Send->toPlainText()), ui->comboBox_DeviceInfo->currentText());
    }else
    {
        //ascii发送
        if(ui->checkBox_AllSend->isChecked())
            server.send(ui->textEdit_Send->toPlainText().toLocal8Bit());
        else
            server.send(ui->textEdit_Send->toPlainText().toLocal8Bit(), ui->comboBox_DeviceInfo->currentText());
    }

}

//自定义发送
void QFormDebug::on_pushButton_CustomSend_clicked()
{
    quint8 addr = ui->spinBox_CustomAddr->value();
    quint8 type = ui->spinBox_CustomFrameType->value();
    QByteArray data;
    //tcp server
    TcpServer& server = TcpServer::getHandle();
    try {
//        MainWindow *ptr = (MainWindow*)parentWidget();//获取父窗口
        data += server.hexStringToByteArray(ui->lineEdit_CustomData->text());
    }  catch (...) {
        //输入数据异常
        return;
    }

    if(ui->pushButton_StartServer->text() == QString("启动服务"))
    {
        QMessageBox::information(this, tr("提示"), tr("请先启动服务"));
        return;
    }

    if(ui->comboBox_DeviceInfo->count() == 0)
    {
        QMessageBox::information(this, tr("提示"), tr("请等待设备连接"));
        return;
    }

    Frame frame(addr, type, data);


    server.send(frame.getFrame(), ui->comboBox_DeviceInfo->currentText());

//    MainWindow *ptr = (MainWindow*)parentWidget();//获取父窗口
    ui->textBrowser_Receive->append("<span style=\" color:#55aaff;\">" + server.byteArrayToHexString(frame.getFrame()) + "</span>");
    if(ui->checkBox_AutoDecode->isChecked())
    {
        //自动解码
        static Protocol protocol;
        protocol.process(frame.getFrame());
        if(!protocol.isEmpty())
        {
            //发送一帧数据
            Frame frame = protocol.getFrame();
            //<html><head/><body><p><span style=" color:#55aa7f;">你好</span></body></html>
            QString str = QString("设备地址：%1 帧类型：%2 数据长度：%3")
                    .arg(frame.addr(), 2, 10, QLatin1Char('0'))
                    .arg(frame.type(), 2, 10, QLatin1Char('0'))
                    .arg(frame.length());
            ui->textBrowser_Receive->append("<span style=\" color:#55aaff;\">" + str + "</span>");
            ui->textBrowser_Receive->append("<span style=\" color:#55aaff;\">" + hexDump(frame.getData(), true) + "</span>");

            qDebug() << "自定义发送 hexdump:\n" << hexDump(frame.getData());
        }
    }
}

//弹出调试命令面板
void QFormDebug::on_pushButton_DebugCmd_clicked()
{
    QFormDebugCmd *formDebugCmd = new QFormDebugCmd(this);
    formDebugCmd->setWindowFlag(Qt::Window,true);
    formDebugCmd->setWindowModality(Qt::WindowModal);//窗口阻塞，但是该函数不会阻塞
    formDebugCmd->show();
}

//切换选项卡
void QFormDebug::on_comboBox_DeviceInfo_currentTextChanged(const QString &arg1)
{
    TcpServer& server = TcpServer::getHandle();
//    if(server.Ip2IdTable.find(arg1) != server.Ip2IdTable.end())
//        ui->label_DeviceId->setText(server.Ip2IdTable[arg1]);
//    ui->label_DeviceId->setText(server.findId(arg1));
    if(ui->comboBox_DeviceInfo->count() != 0)
    {
        const QString& info = arg1;
        QList<QTcpSocket *> list = server.m_server.findChildren<QTcpSocket *>(info);
        if(list.empty())
        {
            ui->label_DeviceId->setText("请稍后切换选项卡或重新进入本界面刷新id");
            return ;//此时设备还未获得id
        }
        TcpSignDevice* device = list[0]->property("TcpSignDevice").value<TcpSignDevice*>();
        ui->label_DeviceId->setText(device->id.isEmpty()?"未找到对应元素":device->id);
    }else
        ui->label_DeviceId->setText("请连接设备获取id");
}

