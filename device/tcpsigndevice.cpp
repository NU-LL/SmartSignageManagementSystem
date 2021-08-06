#include "tcpsigndevice.h"

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QCoreApplication>

#include "ui/mainwindow.h"
#include "ui/qformmain.h"
#include "device/tcpserver.h"

extern MainWindow* mainWindow;


QHash<QString, TcpSignDevice*> TcpSignDevice::DeviceTable;
QHash<quint8, callback_t> TcpSignDevice::CallbackTable;


//=======================序列化===============================
const QString TcpSignDevice::serialization() const
{
    QString str;

    str += this->id;
    str += "\t\t";
    str += this->groupname;
    str += "\t\t";
    str += this->name;
    str += "\t\t";
    str += this->signid;
    str += "\t\t";
    str += this->offline == 1?tr("离线"):tr("在线");
    str += "\t\t";
    str += this->voice == 1?tr("开"):tr("关");
    str += "\t\t";
    str += this->flash == 1?tr("开"):tr("关");
    str += "\t\t";
    str += this->alert == 1?tr("开"):tr("关");
    str += "\n";

    return str;
}

void TcpSignDevice::serialization(QJsonObject &json) const
{
    json["id"] = this->id;
    json["groupname"] = this->groupname;
    json["name"] = this->name;
    json["signid"] = this->signid;
    json["offline"] = this->offline;
    json["voice"] = this->voice;
    json["flash"] = this->flash;
    json["alert"] = this->alert;
}

void TcpSignDevice::deserialization(const QString &str)
{
    //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
    const QStringList& tmpList = str.split(QRegExp("\\s+"),Qt::SkipEmptyParts);
    //赋值
    this->id = tmpList[0];
    this->groupname = tmpList[1];
    this->name = tmpList[2];
    this->signid = tmpList[3];
    this->offline = 1;//默认均为离线
    this->voice = tmpList[5] == tr("开")?1:0;
    this->flash = tmpList[6] == tr("开")?1:0;
    this->alert = tmpList[7] == tr("开")?1:0;
}

void TcpSignDevice::deserialization(const QJsonObject &json)
{
    this->id = json["id"].toString();
    this->groupname = json["groupname"].toString();
    this->name = json["name"].toString();
    this->signid = json["signid"].toString();
    this->offline = 1;//默认均为离线
    this->voice = json["voice"].toInt();
    this->flash = json["flash"].toInt();
    this->alert = json["alert"].toInt();
}

void TcpSignDevice::Init()
{
    static bool initflag = false;
    if(initflag)//确保只初始化一次
        return ;

    //json文件存储
    QFile loadFile(QDir(QFormOptions::ConfigFilePath).absoluteFilePath("table.json"));
    if(!loadFile.exists())
    {
        loadFile.open(QIODevice::WriteOnly);
        loadFile.close();
    }
    if (!loadFile.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open load file : table.json.");
        return;
    }
    QByteArray loadData = loadFile.readAll();
    loadFile.close();
    QJsonDocument loadDoc(QJsonDocument::fromJson(loadData));
//    QJsonDocument loadDoc(QJsonDocument::fromBinaryData(saveData));

    QJsonObject json = loadDoc.object();
    QJsonArray signDevArray = json["TcpSignDevice"].toArray();
    for (int idx = 0; idx < signDevArray.size(); ++idx)
    {
        QJsonObject signDevObj = signDevArray[idx].toObject();
        TcpSignDevice* dev = new TcpSignDevice();
        dev->deserialization(signDevObj);//获取数据区的一行 并进行反序列化
        DeviceTable[dev->id] = dev;//添加到表中
    }

    initflag = true;
}

bool TcpSignDevice::save()
{
    //json文件存储（注意，会清空）
    QFile saveFile(QDir(QFormOptions::ConfigFilePath).absoluteFilePath("table.json"));
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        qWarning("Couldn't open save file : table.json.");
        return false;
    }
    //遍历整个警示语表 存入json
    QJsonObject tableObj;
    QJsonArray signDevArray;
    foreach(TcpSignDevice* dev, DeviceTable)
    {
        //序列化警示语对象 保存到文本中
        QJsonObject signDevObj;
        dev->serialization(signDevObj);
        signDevArray.append(signDevObj);
    }
    tableObj["TcpSignDevice"] = signDevArray;
    QJsonDocument saveDoc(tableObj);
    saveFile.write(saveDoc.toJson());
//    saveFile.write(saveDoc.toBinaryData());
    saveFile.close();

    return true;
}







int TcpSignDevice::sendMessage(const QString& id, quint8 addr, quint8 type, const QByteArray &data, callback_t func, int timeover)
{
    if(!DeviceTable.contains(id))
        return -1;
    if(this->offline == 1)//离线则不发送
        return -1;

    TcpSignDevice* dev = DeviceTable[id];
    QTimer t;
    QEventLoop loop;//创建事件循环
    t.setSingleShot(true);
    connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);  //异步调用超时退出
    connect(dev, &TcpSignDevice::received, &loop, &QEventLoop::quit);  //异步调用完成退出
    //调用异步函数
    if(!CallbackTable.contains(type))//如果没有才能插入
    {
        if(func == nullptr)//注册回调
        {
            CallbackTable[type] = cmd_default_callback;
        }else
            CallbackTable[type] = func;
    }

    //发送数据
    Frame frame(addr, type, data);
    TcpServer::getHandle().send(frame.getFrame(), dev->info);
    t.start(timeover);

    qDebug() << "开始阻塞 接收数据" << dev->info;

    loop.exec();//事件循环开始，阻塞，直到定时时间到或收到received信号
    //不能删除回调
    //如果多设备同时响应同一条指令，删除后会找不到
    //回调函数最好别用匿名函数，如果使用 匿名函数中最好别用栈空间的临时变量
//    disregisterCallback(type);//主动删除回调函数

    qDebug() << "阻塞结束" << dev->info;

    if (t.isActive())
        t.stop();
    else//超时
        return 0;
    return 1;
}






//=======================回调函数===============================

void TcpSignDevice::callbackInit()
{
    if(!CallbackTable.contains(01))
        CallbackTable[01] = cmd_01_callback;
    if(!CallbackTable.contains(72))
        CallbackTable[72] = cmd_72_callback;
    if(!CallbackTable.contains(79))
        CallbackTable[79] = cmd_79_callback;
}

void TcpSignDevice::cmd_default_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data)
{
    Q_UNUSED(dev);
    Q_UNUSED(addr);
    Q_UNUSED(type);
    if(data[0] == '0')
        MainWindow::showMessageBox(QMessageBox::Information, tr("提醒"), QString("发送%1命令成功").arg(type), 2000);
    else
        MainWindow::showMessageBox(QMessageBox::Warning, tr("警告"), QString("设备处理失败[%1命令]").arg(type), 2000);
};
void TcpSignDevice::cmd_72_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data)
{
    Q_UNUSED(addr);
    Q_UNUSED(type);
    QTcpSocket *tcp = (QTcpSocket *)dev;
    TcpSignDevice *device = tcp->property("TcpSignDevice").value<TcpSignDevice *>();

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
    device->offline = 0;//在线
}
void TcpSignDevice::cmd_79_callback(void *_dev, quint8 addr, quint8 type, const QByteArray &data)
{
    Q_UNUSED(addr);
    Q_UNUSED(type);
    QTcpSocket *tcp = (QTcpSocket *)_dev;
    TcpSignDevice *dev = tcp->property("TcpSignDevice").value<TcpSignDevice *>();

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

    dev->id = id;
    dev->name = name;
    dev->location = location;
    dev->ip = tcp->peerAddress().toString();
    dev->port = tcp->peerPort();
    dev->offline = 0;//在线
    //这里无需考虑info info在new时已经指定
}
void TcpSignDevice::cmd_01_callback(void *dev, quint8 addr, quint8 type, const QByteArray &data)
{
    Q_UNUSED(addr);
    Q_UNUSED(type);
    QTcpSocket *tcp = (QTcpSocket *)dev;
    TcpSignDevice *device = tcp->property("TcpSignDevice").value<TcpSignDevice *>();

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
    device->offline = 0;//在线
}





