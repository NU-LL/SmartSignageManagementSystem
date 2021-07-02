#include "tcpserver.h"

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <QCoreApplication>

#include "mainwindow.h"


extern MainWindow* mainWindow;




TcpServer::TcpServer(QObject *parent) : QObject(parent)
{
    //周期定时器 心跳状态包
    HeartbeatTimer = new QTimer(this);
    connect(HeartbeatTimer, &QTimer::timeout, this, &TcpServer::timeout);
}

TcpServer::~TcpServer()
{
    disconnect(HeartbeatTimer, &QTimer::timeout, this, &TcpServer::timeout);
    HeartbeatTimer->stop();
    delete HeartbeatTimer;

    stopServer();
}


//启动服务器
bool TcpServer::startServer(quint16 port)
{
    if(m_server.listen(QHostAddress::AnyIPv4, port))       //只监听IPV4的所有客户端
    {
        initServerSignals();
        return true;
    }
    else
        return false;
}

void TcpServer::stopServer()
{
    if( m_server.isListening() )
    {
        QList<QTcpSocket *> m_tcps = m_server.findChildren<QTcpSocket *>();
        foreach (QTcpSocket *tcp, m_tcps)
        {
            tcp->close();

            disconnect(tcp, static_cast<void (QTcpSocket:: *)(qint64)>(&QTcpSocket::bytesWritten),
                    this, static_cast<void (TcpServer:: *)(qint64)>(&TcpServer::onServerBytesWritten));
            disconnect(tcp, &QTcpSocket::readyRead, this, &TcpServer::onServerDataReady);
            disconnect(tcp, &QTcpSocket::disconnected, this, &TcpServer::onServerDisconnected);
            disconnect(tcp, &QTcpSocket::connected, this, &TcpServer::onServerConnected);
        }
        m_server.close();
        disconnect(&m_server, &QTcpServer::newConnection, this, &TcpServer::onServerNewConnection);
    }
}

qint64 TcpServer::send(const char *data, qint64 maxSize, QString objectName)
{
    int res = 0;
    QList<QTcpSocket *> m_tcps = m_server.findChildren<QTcpSocket *>(objectName);
    foreach (QTcpSocket *tcp, m_tcps)
    {
//        qDebug() << tcp->objectName();
        int temp = tcp->write(data, maxSize);
        res = std::max(temp, res);
        if(-1 == temp)
        {
            qDebug() << "发送给" + tcp->objectName() + "失败，停止本轮数据发送";
            return -1;
        }
    }
    return res;
}

qint64 TcpServer::send(const QByteArray &byteArray, QString objectName)
{
    int res = 0;
    QList<QTcpSocket *> m_tcps = m_server.findChildren<QTcpSocket *>(objectName);
    foreach (QTcpSocket *tcp, m_tcps)
    {
//        qDebug() << tcp->objectName();
        int temp = tcp->write(byteArray);
        res = std::max(temp, res);
        if(-1 == temp)
        {
            qDebug() << "发送给" + tcp->objectName() + "失败，停止本轮数据发送";
            return -1;
        }
    }
    return res;
}



//注册回调函数
bool TcpServer::registerCallback(quint8 type, std::function<void (TcpDevice*, quint8, const QByteArray &)> func)
{
    if(CallbackTable.find(type) == CallbackTable.end())
        CallbackTable[type] = func;//注册
    else
//        QMessageBox::warning(this, tr("警告"), tr("回调函数已经注册：")+type);
        return false;
    return true;
}

//销毁
bool TcpServer::disregisterCallback(quint8 type)
{
    if(CallbackTable.find(type) == CallbackTable.end())
//        QMessageBox::warning(this, tr("警告"), tr("回调函数已经销毁：")+type);
        return false;
    else
        CallbackTable.remove(type);
    return true;
}

//发送数据
int TcpServer::sendMessage_noblock(quint8 addr, quint8 type, const QByteArray &data, QString objectName)
{
    Frame frame(addr, type, data);

    //发送数据
    if(send(frame.getFrame(), objectName) == -1)
        return -1;//发送失败

    return 0;
}


//阻塞式发送 不可重入
//返回值：
//0：正常
//-1：发送失败
//-2：超时
//-3：重入
int TcpServer::sendMessage(quint8 addr, quint8 type, const QByteArray &data, QString objectName, callback_t func, int timeover)
{
    static bool sendflag = false;//重入标志
    callback_completion_flag = false;

    //检查重入
    if(sendflag)
    {
//        QMessageBox::warning(this, tr("警告"), tr("请等待上一条信息发送完成"));
        return -3;
    }

    sendflag = true;//防止重入
    //注册
    if(func == nullptr)
    {
        CallbackTable[type] = [&](TcpDevice*, quint8, const QByteArray& data1)
        {
            if(data1[0] == '0')
                QMessageBox::information(mainWindow, tr("通知"), tr("发送成功"));
            else
                QMessageBox::warning(mainWindow, tr("警告"), tr("设备处理失败"));
            callback_completion_flag = true;//设置回调标志
        };
    }else
        CallbackTable[type] = func;



    Frame frame(addr, type, data);

    //发送数据
    if(send(frame.getFrame(), objectName) == -1)
    {
        sendflag = false;
//        QMessageBox::warning(this, tr("警告"), tr("数据发送失败"));
        QMessageBox::warning(mainWindow, tr("警告"), tr("数据发送失败"));
        return -1;//发送失败
    }


    QTime dieTime = QTime::currentTime().addMSecs(timeover);
    while( QTime::currentTime() < dieTime && callback_completion_flag == false)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);//ms
    }
    if(callback_completion_flag == false)
    {
//        QMessageBox::warning(this, tr("警告"), tr("应答信号超时，类型：") + QString::number(type));
        QMessageBox::warning(mainWindow, tr("警告"), tr("应答信号超时，类型：") + QString::number(type));
        sendflag = false;
        return -2;//超时
    }
    sendflag = false;
    return 0;
}




//hex字符串 转 hex
//"A1 A2" ==> [A1 A2]
QByteArray TcpServer::hexStringToByteArray(const QString hexstring)
{
    bool ok;
    QByteArray ret;
    QString HexString = hexstring;
    HexString = HexString.trimmed();
    HexString = HexString.simplified();
    QStringList sl = HexString.split(" ");

    foreach (QString s, sl) {
        if(!s.isEmpty()) {
            if(s.size() > 2)
            {
                qDebug()<<"待发送的hex数据长度超出2位："<<s;
                QMessageBox::critical(mainWindow, "错误", "待发送的hex数据长度超出2位："+s);
                throw tr("数据异常");
            }
            char c = s.toInt(&ok,16)&0xFF;
            if(ok){
                ret.append(c);
            }else{
                qDebug()<<"非法的16进制字符："<<s;
                QMessageBox::critical(mainWindow, "错误", "非法的16进制字符："+s);
                throw tr("数据异常");
            }
        }
    }
    return ret;
}

//hex 转 hex字符串
//[A1 A2] ==> "A1 A2"
QString TcpServer::byteArrayToHexString(const QByteArray &cba)
{
    QByteArray ba = cba;
    QDataStream out(&ba,QIODevice::ReadWrite);   //将str的数据 读到out里面去
    QString buf;
    while(!out.atEnd())
    {
        qint8 outChar = 0;
        out >> outChar;   //每次一个字节的填充到 outchar
        QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0')).toUpper() + QString(" ");   //2 字符宽度
        buf += str;
    }
    return buf;
}




//===================槽函数=====================

//周期定时器 心跳状态包
void TcpServer::timeout()
{
    if(serverStatus() == true && !DeviceTab.empty())
    {
        QByteArray data;
        for(auto it = DeviceTab.begin(); it != DeviceTab.end(); ++it)
        {
            TcpDevice* device = it.value();
            if(device == nullptr)
            {
                qCritical() << "device == nullptr, key:" << it.key() << " val:" << it.value();
                continue;
            }

            sendMessage(01, data, device->info, [&](TcpDevice* device, quint8, const QByteArray &data){
                if(data[0] != '0')
                {
                    QMessageBox::warning(mainWindow, tr("警告"), QString("设备处理状态信号[01帧]失败，故障信息：0x%1").arg(data[1], 2, 16, QLatin1Char('0')));
                    return ;
                }
                if(data.size() != 12)
                {
                    QMessageBox::warning(mainWindow, tr("警告"), QString("状态信号[01帧]数据长度错误"));
                    return ;
                }
                if(device == nullptr)
                {
                    qDebug() << "device 为空";
                    return ;
                }

                quint8 staFault = data[1];//异常状态
                quint8 staBit = data[2];//状态字
                QString signNum;//警示语标号
                signNum += data[3];
                signNum += data[4];
                signNum += data[5];
                QString imageNum;//图片编号
                imageNum += data[6];
                imageNum += data[7];
                imageNum += data[8];
                quint8 light = data[9];//显示亮度
                quint8 vol = data[10];//语音音量
                quint8 delay = data[11];//提示延时

                //显示异常
                device->stafault = staFault;
                //设置状态
                device->stabyte = staBit;
                device->signidx = signNum.toInt();
                device->imageidx = imageNum.toInt();
                device->light = light;
                device->vol = vol;
                device->delay = delay;

                emit message(INFORMATION, device->ip, QString("%1").arg(device->port), MESSAGE_CHANGE_STATUS);//发送消息 修改界面状态

                //回调完成标志，在 sendMessage 中，如果有回调函数，则完成时一定要设置改标志位
                callback_completion_flag = true;
            });
        }

    }
}

void TcpServer::onServerNewConnection()
{
    QTcpSocket *tcp = m_server.nextPendingConnection();     //获取新的客户端信息
    QString info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
    qDebug() << "连接客户端：" << info;
    emit message(INFORMATION, tr("提示"), QString("新的客户端连入:%1").arg(info));//发送 messagebox
    emit message(INFORMATION, tcp->peerAddress().toString(), QString::number(tcp->peerPort()), MESSAGE_NEWCONNECTION);//发送消息

    tcp->setObjectName(info);       //设置名称

    connect(tcp, &QTcpSocket::connected, this, &TcpServer::onServerConnected);
    connect(tcp, &QTcpSocket::disconnected, this, &TcpServer::onServerDisconnected);
    connect(tcp, &QTcpSocket::readyRead, this, &TcpServer::onServerDataReady);
    connect(tcp, static_cast<void (QTcpSocket:: *)(qint64)>(&QTcpSocket::bytesWritten),
            this, static_cast<void (TcpServer:: *)(qint64)>(&TcpServer::onServerBytesWritten));

    //以下几条sendMessage命令不能使用TcpDevice*，此时该对象为空，还未建立
    //第一条命令 获得id 填写id表
    QByteArray data;
    sendMessage(74, data, info, [&](TcpDevice*, quint8, const QByteArray &data){//阻塞函数
        if(data[0] != '0')
        {
            QMessageBox::warning(mainWindow, tr("警告"), QString("设备处理id信号[74帧]失败"));
            return ;
        }
        if(data.size() != 33)
        {
            QMessageBox::warning(mainWindow, tr("警告"), QString("id信号[74帧]数据长度错误"));
            return ;
        }

        QString id;
        QString password;
        for(int i = 1; i < 17; i++)
            if(data[i] != 0)
                id += data[i];
        for(int i = 17; i < data.size(); i++)
            if(data[i] != 0)
                password += data[i];

        bindTable(info, id);//绑定

        callback_completion_flag = true;//设置回调标志
    });
    disregisterCallback(74);
    //获得名字
    QString name;
    QString location;
    sendMessage(72, data, info, [&](TcpDevice*, quint8, const QByteArray &data){//阻塞函数
        if(data[0] != '0')
        {
            QMessageBox::warning(mainWindow, tr("警告"), QString("设备处理id信号[72帧]失败"));
            return ;
        }

        for(int i = 1; i < 9; i++)
            if(data[i] != 0)
                name += data[i];
        for(int i = 9; i < data.size(); i++)
            if(data[i] != 0)
                location += data[i];

        callback_completion_flag = true;//设置回调标志
    });
    disregisterCallback(72);
    //获得标示语
    QString sign;//读取标示语
//    sendMessage(13, data+"000", info, [&](TcpDevice*, quint8, const QByteArray &data){//阻塞函数
//        if(data[0] != '0')
//        {
//            QMessageBox::warning(mainWindow, tr("警告"), QString("设备处理id信号[13帧]失败"));
//            return ;
//        }
//        sign = data;

//        callback_completion_flag = true;//设置回调标志
//    });
//    disregisterCallback(13);

    //已经获取到设备的必要信息 发送添加设备
    QString id = Ip2IdTable[info];
    DeviceTab[id] = new TcpDevice(info, id, name, (quint8)0);
    DeviceTab[id]->location = location;
    DeviceTab[id]->sign = sign;
    DeviceTab[id]->ip = tcp->peerAddress().toString();
    DeviceTab[id]->port = tcp->peerPort();



    //立马执行超时函数 更新标识位
    timeout();
    //激活定时器 周期性调用超时函数
    HeartbeatTimer->start(timerInterval);

    emit message(INFORMATION, tcp->peerAddress().toString(), QString::number(tcp->peerPort()), MESSAGE_ADDDEVICE);//发送消息
}

void TcpServer::onServerConnected()
{

}

void TcpServer::onServerDisconnected()
{
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    if( tcp != NULL )       //从连接对象中移除掉
    {
        qDebug() << "连接断开，ip：" << tcp->peerAddress().toString() << " 端口：" << tcp->peerPort();
        QString info=tcp->peerAddress().toString()+":"+QString("%1").arg(tcp->peerPort());
        emit message(INFORMATION, tr("提示"), QString("客户端断开连接:%1").arg(info));//发送 messagebox
        emit message(INFORMATION, tcp->peerAddress().toString(), QString::number(tcp->peerPort()), MESSAGE_DISCONNECTION);//发送消息

        QString id = Ip2IdTable[info];
        delete DeviceTab[id];//删除设备对象
        DeviceTab.remove(id);//设备表中移除键值对
        unbindTable(info, id);//解绑
//        QMessageBox::information(this,"提示",QString("客户端断开连接:%1").arg(info),QMessageBox::Ok);
    }
}

void TcpServer::onServerDataReady()
{
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    QByteArray data = tcp->readAll();


    qDebug() << "[接受来自"+ tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort())+"]:";
    qDebug() << QString::fromLocal8Bit(data) << " <==> " << byteArrayToHexString(data);

    //解码
    static Protocol protocol;
    protocol.process(data);
    if(!protocol.isEmpty())
    {
        //接收到一帧数据
        Frame frame = protocol.getFrame();

        qDebug() << QString("设备地址：%1 帧类型：%2 数据长度：%3")
                    .arg(frame.addr(), 2, 10, QLatin1Char('0'))
                    .arg(frame.type(), 2, 10, QLatin1Char('0'))
                    .arg(frame.length());

        if(CallbackTable.find(frame.type()) == CallbackTable.end())
        {
            //该命令未注册
            qWarning() << "该命令未注册";
            return;
        }

        QString info = tcp->objectName();
        QString id;
        if(Ip2IdTable.find(info) != Ip2IdTable.end())
            id = Ip2IdTable[info];
        TcpDevice* device = nullptr;
        if(!id.isEmpty() && DeviceTab.find(id) != DeviceTab.end())
            device = DeviceTab[id];
        //device为空时 出现在刚刚链接时 发送查询信号的过程中，此时这几个查询信号是不能用device的
        if(device == nullptr)
            qCritical() << "获取 TcpDevice* 错误 info:" << info << " id:" << id;


        CallbackTable[frame.type()](device, frame.addr(), frame.getData());
    }

    emit recData(tcp, data);


//    if(tcp->peerAddress().toString()!=targetAddr || tcp->peerPort()!=targetPort  )
//    {
//        targetAddr = tcp->peerAddress().toString();
//        targetPort = tcp->peerPort();
//        ui->recvEdit->insertPlainText("[接受来自"+ targetAddr+":"+QString("%1").arg(targetPort)+"]:\r\n");
//    }
//    ui->recvEdit->moveCursor(QTextCursor::End);
//    ui->recvEdit->insertPlainText(QString::fromLocal8Bit(tcp->readAll())+"\r\n");
}

void TcpServer::onServerBytesWritten(qint64 bytes)
{
    qDebug() << "发送字符:" << bytes;
//    ui->sendLenLabel->setText(QString("%1").arg(ui->sendLenLabel->text().toInt()+bytes));
}


