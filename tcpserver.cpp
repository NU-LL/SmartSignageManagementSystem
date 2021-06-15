#include "tcpserver.h"

#include <qdebug.h>
#include <QMessageBox>



TcpServer::TcpServer(QObject *parent) : QObject(parent)
{

}

TcpServer::~TcpServer()
{
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

//初始化客户端信号槽
void TcpServer::initServerSignals()
{
    connect(&m_server, &QTcpServer::newConnection, this, &TcpServer::onServerNewConnection);
}




//===================槽函数=====================


void TcpServer::onServerNewConnection()
{
    QTcpSocket *tcp = m_server.nextPendingConnection();     //获取新的客户端信息
    QString info = tcp->peerAddress().toString() + ":" + QString("%1").arg(tcp->peerPort());
    qDebug() << "连接客户端：" << info;
    emit message(INFORMATION, tr("提示"), QString("新的客户端连入:%1").arg(info));//发送 messagebox
    emit message(INFORMATION, tcp->peerAddress().toString(), QString::number(tcp->peerPort()), MESSAGE_NEWCONNECTION);//发送消息

//    QMessageBox::information(this,"提示",QString("新的客户端连入:%1").arg(info),QMessageBox::Ok);

    tcp->setObjectName(info);       //设置名称,方便查找

    connect(tcp, &QTcpSocket::connected, this, &TcpServer::onServerConnected);
    connect(tcp, &QTcpSocket::disconnected, this, &TcpServer::onServerDisconnected);
    connect(tcp, &QTcpSocket::readyRead, this, &TcpServer::onServerDataReady);
    connect(tcp, static_cast<void (QTcpSocket:: *)(qint64)>(&QTcpSocket::bytesWritten),
            this, static_cast<void (TcpServer:: *)(qint64)>(&TcpServer::onServerBytesWritten));

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

//        QMessageBox::information(this,"提示",QString("客户端断开连接:%1").arg(info),QMessageBox::Ok);
    }
}

void TcpServer::onServerDataReady()
{
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    emit recData(tcp, tcp->readAll());

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


