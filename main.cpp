#include "ui/mainwindow.h"
#include <iostream>
#include <windows.h>//cout 乱码

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QTextCodec>
#include <QMutex>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include "ui/qdlglogin.h"

MainWindow* mainWindow = nullptr;


void outputMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 加锁
    static QMutex mutex;
    mutex.lock();

    QString strMsg("");
    switch(type)
    {
    case QtDebugMsg:
        strMsg = QString("  Debug ");
        break;
    case QtWarningMsg:
        strMsg = QString(" Warning");
        break;
    case QtCriticalMsg:
        strMsg = QString("Critical");
        break;
    case QtFatalMsg:
        strMsg = QString("  Fatal ");
        break;
    case QtInfoMsg:
        strMsg = QString("  Info  ");
        break;
    }

    // 设置输出信息格式
    QString strDateTime = QDateTime::currentDateTime().toString("hh:mm:ss");
#ifdef QT_NO_DEBUG//release
    QString strMessage = QString("[%1] [%2] : %3")
                .arg(strMsg, 8).arg(strDateTime, 8).arg(msg);
#else
    QString strMessage = QString("[%1||line:%2||%3] [%4] [%5] : %6")
            .arg(QFileInfo(context.file).fileName().left(15), 15)
            .arg(context.line, 5)
            .arg(QString(context.function).left(40), 40)
            .arg(strMsg, 8)
            .arg(strDateTime, 8)
            .arg(msg);
#endif
    // 检查并创建目录
    QDir *tempdir = new QDir;
    if(!tempdir->exists("./log"))
    {
        tempdir->mkdir("./log");
    }
    delete tempdir;
    QString data = QDateTime::currentDateTime().toString ("yyyy-MM-dd");
    QString logfile = "./log/" + data + "-log.txt";

    // 输出信息至文件中（读写、追加形式）
    QFile file(logfile);
    file.open(QIODevice::ReadWrite | QIODevice::Append);
    QTextStream stream(&file);
    stream << strMessage << "\r\n";
    file.flush();
    file.close();

    // 将消息再输出到控制台
    std::cout << strMessage.toLocal8Bit().toStdString() << std::endl;

    // 解锁
    mutex.unlock();
}

int main(int argc, char *argv[])
{
    // 安装消息处理程序
    qInstallMessageHandler(outputMessageHandler);
    qDebug() << "================================new program begin=================================";//日志中换行 便于观察

    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "SmartSignageManagementSystem_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }


//    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
//    QTextCodec::setCodecForLocale(codec); //解决汉字乱码问题

    QDlgLogin *dlgLogin=new QDlgLogin; //创建对话框
    if (dlgLogin->exec()==QDialog::Accepted)
    {
        MainWindow w;
        mainWindow = &w;
        w.show();
        return a.exec();
    }
    else
        return  0;
}
