#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qformoptions.h"
#include "qformdebug.h"
#include "qdlgabout.h"
#include "qformmain.h"

#include <QTime>
#include <QTimer>
#include <QMessageBox>

#include "protocol.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //tcp server
    TcpServer& server = TcpServer::getHandle();
    server.startServer(DEFAULT_PORT);//启动服务器
    //连接tcp信号槽
    connect(&server, static_cast<void (TcpServer:: *)(int, QString, QString, int, void*)>(&TcpServer::message),
            this, static_cast<void (MainWindow:: *)(int, QString, QString, int, void*)>(&MainWindow::recMessage));
    connect(&server, static_cast<void (TcpServer:: *)(QTcpSocket *, const QByteArray& data)>(&TcpServer::recData),
            this, static_cast<void (MainWindow:: *)(QTcpSocket *, const QByteArray& data)>(&MainWindow::recData));


    QTimer::singleShot(10, this, &MainWindow::on_actionMain_triggered);//单次定时器 10ms后触发
}

MainWindow::~MainWindow()
{
    //tcp server
    TcpServer& server = TcpServer::getHandle();
    //断开tcp信号槽
    disconnect(&server, static_cast<void (TcpServer:: *)(QTcpSocket *, const QByteArray& data)>(&TcpServer::recData),
            this, static_cast<void (MainWindow:: *)(QTcpSocket *, const QByteArray& data)>(&MainWindow::recData));
    disconnect(&server, static_cast<void (TcpServer:: *)(int, QString, QString, int, void*)>(&TcpServer::message),
            this, static_cast<void (MainWindow:: *)(int, QString, QString, int, void*)>(&MainWindow::recMessage));
    //停止tcp服务器
    server.stopServer();
    delete ui;
}


////hex字符串 转 hex
////"A1 A2" ==> [A1 A2]
//QByteArray MainWindow::hexStringToByteArray(const QString hexstring)
//{
//    bool ok;
//    QByteArray ret;
//    QString HexString = hexstring;
//    HexString = HexString.trimmed();
//    HexString = HexString.simplified();
//    QStringList sl = HexString.split(" ");

//    foreach (QString s, sl) {
//        if(!s.isEmpty()) {
//            if(s.size() > 2)
//            {
//                qDebug()<<"待发送的hex数据长度超出2位："<<s;
//                QMessageBox::critical(this, "错误", "待发送的hex数据长度超出2位："+s);
//                throw tr("数据异常");
//            }
//            char c = s.toInt(&ok,16)&0xFF;
//            if(ok){
//                ret.append(c);
//            }else{
//                qDebug()<<"非法的16进制字符："<<s;
//                QMessageBox::critical(this, "错误", "非法的16进制字符："+s);
//                throw tr("数据异常");
//            }
//        }
//    }
//    return ret;
//}

////hex 转 hex字符串
////[A1 A2] ==> "A1 A2"
//QString MainWindow::byteArrayToHexString(const QByteArray &cba)
//{
//    QByteArray ba = cba;
//    QDataStream out(&ba,QIODevice::ReadWrite);   //将str的数据 读到out里面去
//    QString buf;
//    while(!out.atEnd())
//    {
//        qint8 outChar = 0;
//        out >> outChar;   //每次一个字节的填充到 outchar
//        QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0')).toUpper() + QString(" ");   //2 字符宽度
//        buf += str;
//    }
//    return buf;
//}




//消息接收槽函数
//level：QMessageBox级别
//      3,2,1,0<==>critical,warning,information,question
void MainWindow::recMessage(int level, QString title, QString text, int message_id, void* message)
{
//    QString& ip = title;
//    QString& port = text;
    //模态显示对话框
    QMessageBox* msgBox = new QMessageBox( this );
    msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
    msgBox->setStandardButtons( QMessageBox::Ok );
    msgBox->setWindowTitle(title);
    msgBox->setText(text);
    msgBox->setModal( false ); // if you want it non-modal

    switch(message_id)
    {
        case(MESSAGE_BOX):
        {
            switch(level)
            {
                case(QUESTION):
                {
                    msgBox->setIcon(QMessageBox::Question);
                    msgBox->show();
                }
//                    QMessageBox::question(this, title, text);
                break;
                case(INFORMATION):
                {
                    msgBox->setIcon(QMessageBox::Information);
                    msgBox->show();
                }
//                    QMessageBox::information(this, title, text);
                break;
                case(WARNING):
                {
                    msgBox->setIcon(QMessageBox::Warning);
                    msgBox->show();
                }
//                    QMessageBox::warning(this, title, text);
                break;
                case(CRITICAL):
                {
                    msgBox->setIcon(QMessageBox::Critical);
                    msgBox->show();
                }
//                    QMessageBox::critical(this, title, text);
                break;
                default:
                    qDebug() << "接收消息类型错误：" << level;
                break;
            }
            break;
        }
    }
}

//消息接收槽函数
void MainWindow::recData(QTcpSocket *tcp, const QByteArray& data)
{
////    QByteArray buf = tcp->readAll();
//    qDebug() << "[接受来自"+ tcp->peerAddress().toString()+":"+QString::number(tcp->peerPort())+"]:";
//    qDebug() << QString::fromLocal8Bit(data) << " <==> " << byteArrayToHexString(data);

//    //解码
//    static Protocol protocol;
//    protocol.process(data);
//    if(!protocol.isEmpty())
//    {
//        //接收到一帧数据
//        Frame frame = protocol.getFrame();

//        qDebug() << QString("设备地址：%1 帧类型：%2 数据长度：%3")
//                    .arg(frame.addr(), 2, 10, QLatin1Char('0'))
//                    .arg(frame.type(), 2, 10, QLatin1Char('0'))
//                    .arg(frame.length());

//        if(CallbackTable.find(frame.type()) == CallbackTable.end())
//        {
//            //该命令未注册
//            qWarning() << "该命令未注册";
//            return;
//        }

//        CallbackTable[frame.type()](frame.addr(), frame.getData());
//    }
}













//启动调试
void MainWindow::on_actionDebug_triggered()
{
    foreach (QMdiSubWindow *window, ui->mdiArea->subWindowList())
    {
        QWidget *mdiChild = (QWidget*)(window);
        if ( mdiChild->windowTitle() == tr("调试"))
        {
            //如果已经打开过，则获得焦点
            mdiChild->setFocus(Qt::MouseFocusReason);
//            mdiChild->showMaximized();
            return ;
        }
    }

    QFormDebug *formDebug = new QFormDebug(this);//需要把this传过去 否则子窗口获取不到父窗口

    ui->mdiArea->addSubWindow(formDebug);
    formDebug->show();
}

//关于
void MainWindow::on_actionAbout_triggered()
{
    QDlgAbout *dlgAbout = new QDlgAbout(this);
    dlgAbout->exec();
    QMessageBox::aboutQt(this, tr("关于"));
}

//选项
void MainWindow::on_actionOptions_triggered()
{
    QFormOptions *formOptions = new QFormOptions(this);
    formOptions->setWindowFlag(Qt::Window,true);

//    formOptions->setWindowFlag(Qt::CustomizeWindowHint,true);
//    formOptions->setWindowFlag(Qt::WindowMinMaxButtonsHint,true);
//    formOptions->setWindowFlag(Qt::WindowCloseButtonHint,true);
//    formOptions->setWindowFlag(Qt::WindowStaysOnTopHint,true);
//    formOptions->setWindowState(Qt::WindowMaximized);
//    formOptions->setWindowOpacity(0.9);
//    formOptions->setWindowModality(Qt::WindowModal);//窗口阻塞，但是该函数不会阻塞


    formOptions->show();
}

//主界面
void MainWindow::on_actionMain_triggered()
{
    foreach (QMdiSubWindow *window, ui->mdiArea->subWindowList())
    {
        QWidget *mdiChild = (QWidget*)(window);
        if ( mdiChild->windowTitle() == tr("主界面"))
        {
            //如果已经打开过，则获得焦点
            mdiChild->setFocus(Qt::MouseFocusReason);
//            mdiChild->showMaximized();
            return ;
        }
    }

//    static QFormMain *formMain = nullptr;

//    if(formMain != QFormMain::instance())
//        formMain = QFormMain::instance();
//    else
//        return;

    QFormMain *formMain = new QFormMain(this);//需要把this传过去 否则子窗口获取不到父窗口
    ui->mdiArea->addSubWindow (formMain);
    formMain->show();
}



