#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "qformoptions.h"
#include "qformdebug.h"
#include "qdlgabout.h"
#include "ui/qformmain.h"

#include <QTime>
#include <QTimer>
#include <QMessageBox>

#include "protocol/protocol.h"


extern MainWindow* mainWindow;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //初始化配置文件
    QFormOptions::Init();
    if(QFormOptions::defaultStartServer)//根据配置决定是否自动联网
    {
        //tcp server
        TcpServer& server = TcpServer::getHandle();
        server.startServer(DEFAULT_PORT);//启动服务器
        //连接tcp信号槽
        connect(&server, static_cast<void (TcpServer:: *)(int, void*)>(&TcpServer::message),
                this, static_cast<void (MainWindow:: *)(int, void*)>(&MainWindow::recMessage));
        connect(&server, static_cast<void (TcpServer:: *)(QTcpSocket *, const QByteArray& data)>(&TcpServer::recData),
                this, static_cast<void (MainWindow:: *)(QTcpSocket *, const QByteArray& data)>(&MainWindow::recData));
    }


    QTimer::singleShot(10, this, &MainWindow::on_actionMain_triggered);//单次定时器 10ms后触发
}

MainWindow::~MainWindow()
{
    //tcp server
    TcpServer& server = TcpServer::getHandle();
    //断开tcp信号槽
    disconnect(&server, static_cast<void (TcpServer:: *)(QTcpSocket *, const QByteArray& data)>(&TcpServer::recData),
            this, static_cast<void (MainWindow:: *)(QTcpSocket *, const QByteArray& data)>(&MainWindow::recData));
    disconnect(&server, static_cast<void (TcpServer:: *)(int, void*)>(&TcpServer::message),
            this, static_cast<void (MainWindow:: *)(int, void*)>(&MainWindow::recMessage));
    //停止tcp服务器
    server.stopServer();
    delete ui;
}




void MainWindow::showMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text, int ms, bool isBlock)
{
    //模态显示对话框
    QMessageBox* msgBox = new QMessageBox( mainWindow );
    msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
    msgBox->setStandardButtons( QMessageBox::Ok );
    msgBox->setWindowTitle(title);
    msgBox->setText(text);
    msgBox->setModal( false ); // if you want it non-modal
    msgBox->setIcon(icon);
    if(ms != 0)
        QTimer::singleShot(ms, msgBox, &QMessageBox::accept);//定时关闭 ms
    if(isBlock)
        msgBox->exec();
    else
        msgBox->show();
}

void MainWindow::showStatusText(const QString &text, int ms)
{
    mainWindow->ui->statusbar->showMessage(text, ms);
}






//消息接收槽函数
//level：QMessageBox级别
//      3,2,1,0<==>critical,warning,information,question
void MainWindow::recMessage(int type, void* message)
{
    Q_UNUSED(type);Q_UNUSED(message);
}

//消息接收槽函数
void MainWindow::recData(QTcpSocket *tcp, const QByteArray& data)
{
    Q_UNUSED(tcp);Q_UNUSED(data);
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










//获得调试窗口的句柄
QWidget *MainWindow::getQFormDebug()
{
    foreach (QMdiSubWindow *window, ui->mdiArea->subWindowList())
    {
        QWidget *mdiChild = (QWidget*)(window);
        if ( mdiChild->windowTitle() == tr("调试"))
        {
            return mdiChild;
        }
    }
    return nullptr;
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



