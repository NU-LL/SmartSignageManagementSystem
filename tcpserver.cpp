#include "tcpserver.h"

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QCoreApplication>

#include "mainwindow.h"
#include "qformmain.h"

extern MainWindow* mainWindow;


QString TcpServer::ERROR = QString(tr("未找到对应元素"));


TcpServer::TcpServer(QObject *parent) : QObject(parent)
{
    //周期定时器 心跳状态包
    HeartbeatTimer = new QTimer(this);
    connect(HeartbeatTimer, &QTimer::timeout, this, static_cast<void (TcpServer:: *) (void)>(&TcpServer::timeout));
}

TcpServer::~TcpServer()
{
//    disconnect(HeartbeatTimer, &QTimer::timeout, nullptr, nullptr);//断开timeout的所有连接
    disconnect(HeartbeatTimer, &QTimer::timeout, this, static_cast<void (TcpServer:: *) (void)>(&TcpServer::timeout));
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

//qint64 TcpServer::send(const char *data, qint64 maxSize, const QString& objectName)
//{
//    int res = 0;
//    QList<QTcpSocket *> m_tcps = m_server.findChildren<QTcpSocket *>(objectName);
//    foreach (QTcpSocket *tcp, m_tcps)
//    {
////        qDebug() << tcp->objectName();
//        int temp = tcp->write(data, maxSize);
//        res = std::max(temp, res);
//        if(-1 == temp)
//        {
//            qDebug() << "发送给" + tcp->objectName() + "失败，停止本轮数据发送";
//            return -1;
//        }
//    }
//    return res;
//}

qint64 TcpServer::send(const QByteArray &byteArray, const QString& objectName)
{
    int res = 0;
    QList<QTcpSocket *> m_tcps = m_server.findChildren<QTcpSocket *>(objectName);
    foreach (QTcpSocket *tcp, m_tcps)
    {
//        qDebug() << tcp->objectName();
        int temp = tcp->write(byteArray);
        qDebug();
        qDebug() << "server ==> ["+ tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort())+"]:";
        qDebug() << QString::fromLocal8Bit(byteArray) << " ---- " << byteArrayToHexString(byteArray);
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
bool TcpServer::registerCallback(quint8 type, std::function<void (void*, quint8, const QByteArray &)> func)
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
int TcpServer::sendMessage_noblock(quint8 addr, quint8 type, const QByteArray& data, const QString& objectName)
{
    Frame frame(addr, type, data);

    //发送数据
    if(send(frame.getFrame(), objectName) == -1)
        return -1;//发送失败

    return 0;
}


//阻塞式发送
int TcpServer::sendMessage(TcpDevice* tcpdev, quint8 addr, quint8 type, const QByteArray &data, callback_t func, int timeover)
{
    if(tcpdev == nullptr)
        return -1;
    QTimer t;
    QEventLoop loop;//创建事件循环
    t.setSingleShot(true);
    connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);  //异步调用超时退出
    connect(tcpdev, &TcpDevice::received, &loop, &QEventLoop::quit);  //异步调用完成退出
//    connect(tcpdev, &TcpDevice::received_queue, &loop, &QEventLoop::quit, Qt::QueuedConnection);  //异步调用完成退出
    //调用异步函数
    if(func == nullptr)//注册回调
    {
        CallbackTable[type] = [type](void*, quint8, const QByteArray& data1)
        {
            if(data1[0] == '0')
                MainWindow::showMessageBox(QMessageBox::Information, tr("提醒"), QString("发送%1命令成功").arg(type), 2000);
            else
                MainWindow::showMessageBox(QMessageBox::Warning, tr("警告"), QString("设备处理失败[%1命令]"), 2000);
        };
    }else
        CallbackTable[type] = func;
//    if(func != nullptr)
//    {
//        qDebug() << "命令：" << type << " 回调函数地址：" << &func;
//    }
    sendMessage_noblock(addr, type, data, tcpdev->info);
    t.start(timeover);

    qDebug() << tcpdev->info << " 等待接收数据";

    loop.exec();//事件循环开始，阻塞，直到定时时间到或收到received信号
    //不能删除回调
    //如果多设备同时响应同一条指令，删除后会找不到
    //回调函数最好别用匿名函数，如果使用 匿名函数中最好别用栈空间的临时变量
//    disregisterCallback(type);//主动删除回调函数

    qDebug() <<"阻塞结束";

    if (t.isActive())
        t.stop();
    else//超时
        return 0;
    return 1;
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
                MainWindow::showMessageBox(QMessageBox::Critical, "错误", "待发送的hex数据长度超出2位："+s);
                throw tr("数据异常");
            }
            char c = s.toInt(&ok,16)&0xFF;
            if(ok){
                ret.append(c);
            }else{
                qDebug()<<"非法的16进制字符："<<s;
                MainWindow::showMessageBox(QMessageBox::Critical, "错误", "非法的16进制字符："+s);
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







//===================命令回调函数=====================
void TcpServer::cmd_72_callback(void *dev, quint8 addr, const QByteArray &data)
{
    Q_UNUSED(addr);
    QTcpSocket *tcp = (QTcpSocket *)dev;
    TcpDevice *device = tcp->property("TcpDevice").value<TcpDevice *>();

    if(data[0] != '0')
    {
        MainWindow::showMessageBox(QMessageBox::Warning, tr("警告"), "读取警示牌的名称和安装位置[72帧]失败");
        return ;
    }
    if(data.size() != 33)
    {
        MainWindow::showMessageBox(QMessageBox::Warning, tr("警告"), "读取警示牌的名称和安装位置[72帧]数据长度错误");
        return ;
    }

    QString name;
    QString location;
    int pos = 1;
    //name
    for(int i = 0; i < 16; i++)
        if(data[pos+i] != 0)
            name += data[pos+i];
    pos += 16;
    //location
    for(int i = 0; i < 16; i++)
        if(data[pos+i] != 0)
            location += data[pos+i];

    device->name = name;
    device->location = location;
}
void TcpServer::cmd_79_callback(void *dev, quint8 addr, const QByteArray &data)
{
    Q_UNUSED(addr);
    QTcpSocket *tcp = (QTcpSocket *)dev;
    TcpDevice *tcpdev = tcp->property("TcpDevice").value<TcpDevice *>();
    QString info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
    TcpServer& server = TcpServer::getHandle();

    if(data[0] != '0')
    {
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", "设备处理警示牌信息[79帧]失败");
        return ;
    }
    if(data.size() != 0x31)
    {
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", "警示牌信息[79帧]数据长度错误");
        return ;
    }

    QString name;
    QString location;
    QString id;
    QString password;
    int pos = 1;
    //id
    for(int i = 0; i < 8; i++)
        if(data[pos+i] != 0)
            id += data[pos+i];
    pos += 8;
    //password
    for(int i = 0; i < 8; i++)
        if(data[pos+i] != 0)
            password += data[pos+i];
    pos += 8;
    //name
    for(int i = 0; i < 16; i++)
        if(data[pos+i] != 0)
            name += data[pos+i];
    pos += 16;
    //location
    for(int i = 0; i < 16; i++)
        if(data[pos+i] != 0)
            location += data[pos+i];

    //如果该设备已经存在，需要提醒
    if(server.findIp(id) != server.ERROR)
    {
        SignDevice* signdev = SignDevice::findSignDev(id);
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", "警示牌 "+signdev->name+" 和新警示牌 "+name+" id冲突("+id+")新警示牌不会添加");
        tcp->close();//主动关闭链接
    }else
    {
        server.bindTable(info, id);//绑定

        server.DeviceTab[id] = tcpdev;
        server.DeviceTab[id]->id = id;
        server.DeviceTab[id]->name = name;
        server.DeviceTab[id]->location = location;
        server.DeviceTab[id]->ip = tcp->peerAddress().toString();
        server.DeviceTab[id]->port = tcp->peerPort();
        server.DeviceTab[id]->info = info;
    }
}
void TcpServer::cmd_01_callback(void *dev, quint8 addr, const QByteArray &data)
{
    Q_UNUSED(addr);
    QTcpSocket *tcp = (QTcpSocket *)dev;
    TcpDevice *device = tcp->property("TcpDevice").value<TcpDevice *>();

    if(data[0] != '0')
    {
        MainWindow::showMessageBox(QMessageBox::Warning, tr("警告"), QString("设备处理状态信号[01帧]失败，故障信息：0x%1").arg(data[1], 2, 16, QLatin1Char('0')));
        return ;
    }
    if(data.size() != 0x0d)
    {
        MainWindow::showMessageBox(QMessageBox::Warning, tr("警告"), "状态信号[01帧]数据长度错误");
        return ;
    }
    if(device == nullptr)
    {
        qDebug() << "device 为空";
        return ;
    }

    quint8 staFault = data[1];//异常状态
    quint8 staBit = data[2];//状态字
    quint8 color = data[3];//颜色
    quint8 light = data[4];//亮度
    quint8 vol = data[5];//音量
    quint8 delay = data[6];//报警时间
    QString signNum;//警示语标号
    signNum += data[7];
    signNum += data[8];
    signNum += data[9];
    QString imageNum;//图片编号
    imageNum += data[10];
    imageNum += data[11];
    imageNum += data[12];

    //显示异常
    device->stafault = staFault;
    //设置状态
    device->stabyte = staBit;
    device->signid = signNum;
    device->imageidx = imageNum;
    device->light = light;
    device->vol = vol;
    device->delay = delay;
    device->color = color;
}

//===================槽函数=====================

//周期定时器 心跳状态包
void TcpServer::timeout()
{
    if(serverStatus() == true && !DeviceTab.empty())
    {
        static int idx = 0;
        //保证idx出一定有数据
        if(DeviceTab.size()-1 < idx)
        {
            //说明在上一次发送过程中有节点断开链接
            idx = 0;
            return ;
        }

        //这种方式在第一次与节点插入删除时可能不会发送到，但是稳定后是一定可以发送到的
        auto it = DeviceTab.begin()+idx;
        if(it == DeviceTab.end())
        {
            idx = 0;
            return ;
        }
        TcpDevice* device = it.value();
        QString id = it.key();
        if(device == nullptr)
        {
            qCritical() << "device == nullptr, idx:" << idx << " key:" << it.key() << " val:" << it.value();
            return ;
        }
        //初始化才能发送数据
        if(device->init)
        {
            // 仅仅支持服务器主动发送 不支持下位机主动上报
            QByteArray data;
            //一般状态
//            sendMessage(device, 00, 01, data, cmd_01_callback);
            sendMessage(id, 00, 01, data, cmd_01_callback);
            //名称、安装位置
//            sendMessage(device, 00, 72, data, cmd_72_callback);
            sendMessage(id, 00, 72, data, cmd_72_callback);


            emit message(MESSAGE_CHANGE_STATUS, findTcpDevice(id));//发送消息 修改界面状态
        }


        idx++;
        if(idx > DeviceTab.size()-1)
            idx = 0;
    }
}

void TcpServer::onServerNewConnection()
{
    QTcpSocket *tcp = m_server.nextPendingConnection();     //获取新的客户端信息
    QString info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
    qDebug() << "连接客户端：" << info;
    MainWindow::showMessageBox(QMessageBox::Information, tr("提示"), QString("新的客户端连入:%1").arg(info), 2000);
    emit message(MESSAGE_NEWCONNECTION, &info);//发送消息

    tcp->setObjectName(info);//设置名称

    connect(tcp, &QTcpSocket::connected, this, &TcpServer::onServerConnected);
    connect(tcp, &QTcpSocket::disconnected, this, &TcpServer::onServerDisconnected);
    connect(tcp, &QTcpSocket::readyRead, this, &TcpServer::onServerDataReady);
    connect(tcp, static_cast<void (QTcpSocket:: *)(qint64)>(&QTcpSocket::bytesWritten),
            this, static_cast<void (TcpServer:: *)(qint64)>(&TcpServer::onServerBytesWritten));

    TcpDevice* tcpdev = new TcpDevice(info, "", "", (quint8)0);
    tcp->setProperty("TcpDevice", QVariant::fromValue(tcpdev));//直接将TcpDevice*放到对应的tcp中
    QByteArray data;
    int timeover = REC_TIMEOUT;
    //超时重发
    while(0 == sendMessage(tcpdev, 00, 79, data, cmd_79_callback, timeover))
    {
        timeover = (timeover>=timerInterval/2)?(timerInterval/2):(timeover*2);
        qDebug() << "79命令超时";
    };

    timeover = REC_TIMEOUT;
    //超时重发
    while(0 == sendMessage(tcpdev, 00, 01, data, cmd_01_callback, timeover))
    {
        timeover = (timeover>=timerInterval/2)?(timerInterval/2):(timeover*2);
        qDebug() << "01命令超时";
    };

    //已经获取到设备的必要信息 发送添加设备
    QString id = findId(info);
    if(id != ERROR)
    {
        //激活定时器 周期性调用超时函数
        if(!HeartbeatTimer->isActive())
            HeartbeatTimer->start(timerInterval);

        emit message(MESSAGE_ADDDEVICE, DeviceTab[id]);//发送消息
        DeviceTab[id]->init = true;//初始化完成
    }
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
        MainWindow::showMessageBox(QMessageBox::Information, tr("提示"), QString("客户端断开连接:%1").arg(info), 2000);
        emit message(MESSAGE_DISCONNECTION, findTcpDevice_ip(info));//发送消息


        QString id = Ip2IdTable[info];
//        qDebug() << "发送删除对象信号";
//        emit DeviceTab[id]->received_queue();//删除对象前需要退出所有阻塞的send函数
        delete DeviceTab[id];//删除设备对象
        DeviceTab.remove(id);//设备表中移除键值对
        unbindTable(info, id);//解绑
        tcp->deleteLater();//直接删除该tcp对象
        qDebug() << "对象已经删除";
    }

    if(DeviceTab.empty())
    {
        //设备全部掉线 停止定时器
        HeartbeatTimer->stop();
    }
}

void TcpServer::onServerDataReady()
{
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    QByteArray data = tcp->readAll();

    qDebug();
    qDebug() << "server <== ["+ tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort())+"]:";
    qDebug() << QString::fromLocal8Bit(data) << " ---- " << byteArrayToHexString(data);

    emit recData(tcp, data);

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

        CallbackTable[frame.type()](tcp, frame.addr(), frame.getData());

        //直接从QTcpSocket*中获得对应的TcpDevice*
        TcpDevice* device = tcp->property("TcpDevice").value<TcpDevice*>();
        if(device == nullptr)
            qCritical() << "获取 TcpDevice* 错误，请检测是否内存已满";
        else
        {
//            CallbackTable[frame.type()](tcp, frame.addr(), frame.getData());
            emit device->received();//发送接收完成信号 用于同步 sendMessage
        }


//        QString info = tcp->objectName();
//        QString id = findId(info);
//        TcpDevice* device = findTcpDevice(id);

//        //数据有效性检查
//        //注意：
//        //device为空时 出现在刚刚链接时 发送查询信号的过程中，此时这几个查询信号是不能用device的
//        //所以下明只报警告 不退出该函数
//        if(id == TcpServer::ERROR || device == nullptr)
//        {
//            qCritical() << "获取 TcpDevice* 错误 info:" << info << " id:" << id << "（在新链接建立时会出现一次该警告，此时设备对象还未建立，属于正常现象）";

//            CallbackTable[frame.type()](tcp, frame.addr(), frame.getData());
//            //经过回调 TcpDevice 已经注册成功 更新device
//            id = findId(info);
//            device = findTcpDevice(id);
//            if(device != nullptr)
//                qInfo() << "建立对象，info：" << info << " id:" << id << " name:" << device->name;
//            else
//                qInfo() << "建立对象，info：" << info << " id:" << id << " name:" << "device无效";
//        }else
//            CallbackTable[frame.type()](device, frame.addr(), frame.getData());

//        if(device != nullptr)
//            emit device->received();//发送接收完成信号 用于同步 sendMessage
    }




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
    Q_UNUSED(bytes);
//    qDebug() << "发送字符:" << bytes;
//    ui->sendLenLabel->setText(QString("%1").arg(ui->sendLenLabel->text().toInt()+bytes));
}


