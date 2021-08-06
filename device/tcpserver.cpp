#include "device/tcpserver.h"

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QCoreApplication>

#include "ui/mainwindow.h"
#include "ui/qformmain.h"
#include "ui/qformdebug.h"

extern MainWindow* mainWindow;


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



//发送数据
int TcpServer::sendMessage_noblock(quint8 addr, quint8 type, const QByteArray& data, const QString& objectName)
{
    Frame frame(addr, type, data);

    //发送数据
    if(send(frame.getFrame(), objectName) == -1)
        return -1;//发送失败

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



//===================槽函数=====================

//周期定时器 心跳状态包
void TcpServer::timeout()
{
    QList<QTcpSocket *> list = m_server.findChildren<QTcpSocket *>();
    if(serverStatus() == true && !list.empty())
    {
        static int idx = 0;
        //保证idx出一定有数据
        if(idx > list.size()-1)
        {
            //说明在上一次发送过程中有节点断开链接
            idx = 0;
            return ;
        }

        //这种方式在第一次与节点插入删除时可能不会发送到，但是稳定后是一定可以发送到的
        QTcpSocket *tcp = list[idx];
        TcpSignDevice* dev = tcp->property("TcpSignDevice").value<TcpSignDevice*>();
        //初始化才能发送数据
        if(dev->init)
        {
            //一般状态
            dev->sendMessage(00, 01);
            //名称、安装位置
            dev->sendMessage(00, 72);

            emit message(MESSAGE_CHANGE_STATUS, dev);//发送消息 修改界面状态
        }

        idx++;
        if(idx > list.size()-1)
            idx = 0;
    }
}

void TcpServer::onServerNewConnection()
{
    QTcpSocket *tcp = m_server.nextPendingConnection();     //获取新的客户端信息
    QString info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
    qDebug() << "连接客户端：" << info;
    QFormDebug::showMessageBox(QMessageBox::Information, tr("提示"), QString("新的客户端连入:%1").arg(info), 2000);
    MainWindow::showStatusText(QString("新的客户端连入:%1").arg(info));
    emit message(MESSAGE_NEWCONNECTION, &info);//发送消息

    tcp->setObjectName(info);//设置名称

    connect(tcp, &QTcpSocket::connected, this, &TcpServer::onServerConnected);
    connect(tcp, &QTcpSocket::disconnected, this, &TcpServer::onServerDisconnected_before);
    connect(tcp, &QTcpSocket::disconnected, this, &TcpServer::onServerDisconnected, Qt::QueuedConnection);
    connect(tcp, &QTcpSocket::readyRead, this, &TcpServer::onServerDataReady);
    connect(tcp, static_cast<void (QTcpSocket:: *)(qint64)>(&QTcpSocket::bytesWritten),
            this, static_cast<void (TcpServer:: *)(qint64)>(&TcpServer::onServerBytesWritten));



//    TcpSignDevice* dev = new TcpSignDevice(tcp, info, info, "", (quint8)0);//默认设备名与id为info，默认名称为空
    TcpSignDevice* dev = new TcpSignDevice(tcp, info, "", (quint8)0);//默认id为info，默认名称为空，默认在线
//    dev->ip = tcp->peerAddress().toString();//提前确定ip与端口
//    dev->port = tcp->peerPort();
    tcp->setProperty("TcpSignDevice", QVariant::fromValue(dev));//直接将TcpDevice*放到对应的tcp中
    TcpSignDevice::DeviceTable[info] = dev;//直接插入到表中（此时默认id为info，等到获得这个id再修改）（info肯定不会冲突）

    //超时重发
    int timeover = REC_TIMEOUT;
    int times = 5;
    while(0 == dev->sendMessage(info, 00, 79, QByteArray(), nullptr, timeover))
    {
        timeover = (timeover>=timerInterval/2)?(timerInterval/2):(timeover*2);
        qDebug() << "79命令超时";
        bool exit = false;
        try {
            if(dev->q_tcp.empty())
            {
                exit = true;
                qDebug() << "tcp数量为0";
            }
            else if(!dev->q_tcp.last()->isValid())
            {
                exit = true;//tcp无效
                qDebug() << "tcp无效";
            }
        }  catch (...) {
            exit = true;//此时tcp指向的对象已经删除 必须要退出
            qDebug() << "访问q_tcp异常";
        }
        if(--times <= 0)
            exit = true;
        if(exit)
        {
            TcpSignDevice::DeviceTable.remove(dev->info);
            delete dev;
            if(!dev->q_tcp.empty())
                dev->q_tcp.last()->close();//会发射disconnected信号 在绑定的槽函数中最终会释放tcp
            return;
        }
    };
    //检查id是否存在
    bool isnew = false;//是否新设备
    if(TcpSignDevice::DeviceTable.contains(dev->id))
    {
        //此时已经有id 将数据移到之前的id中
        TcpSignDevice* exist_dev = TcpSignDevice::DeviceTable[dev->id];
        //先报警
        QString warn_str = QString("警示牌%1[%2]和新警示牌%3[%4] id(%5)冲突，有可能是设备重连")
                .arg(dev->name).arg(dev->info)
                .arg(exist_dev->name).arg(exist_dev->info)
                .arg(dev->id);
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", warn_str);
        qDebug() << warn_str;
        //根据79命令 同步老的对象中的值 并设置为在线
        dev->q_tcp.head()->setProperty("TcpSignDevice", QVariant::fromValue(exist_dev));//重设 QTcpSocket 中绑定的对象
        exist_dev->name = dev->name;
        exist_dev->location = dev->location;
        exist_dev->ip = dev->ip;
        exist_dev->port = dev->port;
        exist_dev->info = dev->info;
        if(!dev->q_tcp.isEmpty())//如果非空 则移出最开始的一个
            exist_dev->q_tcp.enqueue(dev->q_tcp.dequeue());//删除q_tcp队列第一个元素,并返回这个元素
        else
            qWarning() << "id重复，新的tcp队列为空";
        exist_dev->offline = 0;//在线
        //删除之前的class
        TcpSignDevice::DeviceTable.remove(dev->info);
        delete dev;
        //重新赋值dev 便于之后使用
        dev = exist_dev;
    }else
    {
        //新的设备接入
        //将原来的id为info更换为真正的id
        TcpSignDevice::DeviceTable[dev->id] = dev;
        TcpSignDevice::DeviceTable.remove(dev->info);
        isnew = true;
    }

    timeover = REC_TIMEOUT;
    times = 5;
    //超时重发
    while(0 == dev->sendMessage(dev->id, 00, 01, QByteArray(), nullptr, timeover))
    {
        timeover = (timeover>=timerInterval/2)?(timerInterval/2):(timeover*2);
        qDebug() << "01命令超时";
        bool exit = false;
        try {
            if(dev->q_tcp.empty())
            {
                exit = true;
                qDebug() << "tcp数量为0";
            }
            else if(!dev->q_tcp.last()->isValid())
            {
                exit = true;//tcp无效
                qDebug() << "tcp无效";
            }
        }  catch (...) {
            exit = true;//此时tcp指向的对象已经删除 必须要退出
            qDebug() << "访问q_tcp异常";
        }
        if(--times <= 0)
            exit = true;
        if(exit)
        {
            TcpSignDevice::DeviceTable.remove(dev->id);//此时变为了id
            delete dev;
            if(!dev->q_tcp.empty())
                dev->q_tcp.last()->close();//会发射disconnected信号 在绑定的槽函数中最终会释放tcp
            return;
        }
    };

    if(TcpSignDevice::DeviceTable.contains(dev->id))
    {
        //激活定时器 周期性调用超时函数
        if(!HeartbeatTimer->isActive())
            HeartbeatTimer->start(timerInterval);

        //是否是新设备
        if(isnew)
            emit message(MESSAGE_ADDDEVICE, TcpSignDevice::DeviceTable[dev->id]);//发送消息
        else
            emit message(MESSAGE_CHANGE_STATUS, TcpSignDevice::DeviceTable[dev->id]);//发送消息
        TcpSignDevice::DeviceTable[dev->id]->init = true;//初始化完成
    }
}

//连接host后才发送该信号（用于client）
void TcpServer::onServerConnected()
{
}

//用于获得精确的ip与断开
//删除debug窗口信息用
//此时tcp还没有断开 里边的信息（ip、port）还能用
void TcpServer::onServerDisconnected_before()
{
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    if( tcp != NULL )       //从连接对象中移除掉
    {
        //弹窗报警
        QString str = QString("连接断开，ip：%1 端口%2").arg(tcp->peerAddress().toString()).arg(tcp->peerPort());
        QFormDebug::showMessageBox(QMessageBox::Information, tr("提示"), str, 2000);
        qDebug() << str;

        //发送断开消息
        QString info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
        emit message(MESSAGE_DISCONNECTION_INFO, &info);//发送消息
    }
}

//此时tcp已经断开 里边的信息（ip、port）已经不存在
void TcpServer::onServerDisconnected()
{
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    if( tcp != NULL )       //从连接对象中移除掉
    {
        TcpSignDevice* device = tcp->property("TcpSignDevice").value<TcpSignDevice*>();

//        //弹窗报警
//        QString str;
//        if(tcp->state() == QAbstractSocket::ConnectedState)
//            str = QString("连接断开，ip：%1 端口%2").arg(tcp->peerAddress().toString()).arg(tcp->peerPort());
//        else
//            str = QString("连接断开，ip：%1 端口%2（如果该设备多id同时登录，ip与端口信息可能有误）").arg(device->ip).arg(device->port);
//        MainWindow::showMessageBox(QMessageBox::Information, tr("提示"), str, 2000);
//        qDebug() << str;

        //更新断开状态
        if(!device->q_tcp.isEmpty())//如果非空 则移出最开始的一个
        {
            device->q_tcp.dequeue();
            if(device->q_tcp.isEmpty())//移除之后为空 说明该节点已经断线（避免多个节点同一个id的情况）
            {
                device->offline = 1;//离线
                device->init = false;//初始化未成功
            }else//否则说明有多个tcp链接
            {

            }
        }else
            qWarning() << "断开节点时，tcp list为空";

        emit message(MESSAGE_CHANGE_STATUS, device);//发送刷新状态消息

        tcp->deleteLater();//直接删除该tcp对象
        qDebug() << "对象已经删除";
    }

    if(isClientEmpty())
    {
        //设备全部掉线 停止定时器
        HeartbeatTimer->stop();
        qDebug() << "连接设备为空，停止心跳包发送";
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


        if(!TcpSignDevice::CallbackTable.contains(frame.type()))
        {
            //该命令未注册
            qWarning() << "该命令未注册";
            return;
        }

        TcpSignDevice::CallbackTable[frame.type()](tcp, frame.addr(), frame.type(), frame.getData());

        //直接从QTcpSocket*中获得对应的TcpDevice*
        TcpSignDevice* device = tcp->property("TcpSignDevice").value<TcpSignDevice*>();
        if(device == nullptr)
            qCritical() << "获取 TcpDevice* 错误，请检测是否内存已满";
        else
        {
            emit device->received();//发送接收完成信号 用于同步 sendMessage
        }
    }
}

void TcpServer::onServerBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
//    qDebug() << "发送字符:" << bytes;
//    ui->sendLenLabel->setText(QString("%1").arg(ui->sendLenLabel->text().toInt()+bytes));
}


