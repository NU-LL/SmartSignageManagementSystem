#include "qformmain.h"
#include "ui_qformmain.h"

#include "config.h"


#include <QDebug>
#include <QStandardItemModel>
#include <QItemSelectionModel>

#include <QTimer>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QRegExp>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>

#include "protocol.h"
#include "mainwindow.h"

#include "qformsigntable.h"
#include "qdialogadddev.h"
#include "qdialogsetsigndev.h"

extern MainWindow* mainWindow;


QHash<QString, SignDevice*> SignDevice::SignDeviceTable;


//序列化
const QString SignDevice::serialization() const
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

void SignDevice::serialization(QJsonObject &json) const
{
//    QJsonObject signdevObj;
    json["id"] = this->id;
    json["groupname"] = this->groupname;
    json["name"] = this->name;
    json["signid"] = this->signid;
    json["offline"] = this->offline;
    json["voice"] = this->voice;
    json["flash"] = this->flash;
    json["alert"] = this->alert;
//    json["SignDevice"] = signdevObj;
}

//反序列化
void SignDevice::deserialization(const QString &str)
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

void SignDevice::deserialization(const QJsonObject &json)
{
//    QJsonObject signdevObj = json["SignDevice"].toObject();
    this->id = json["id"].toString();
    this->groupname = json["groupname"].toString();
    this->name = json["name"].toString();
    this->signid = json["signid"].toString();
    this->offline = 1;//默认均为离线
    this->voice = json["voice"].toInt();
    this->flash = json["flash"].toInt();
    this->alert = json["alert"].toInt();
}

//初始化列表
//需要一开始就调用，获得全局唯一的设备列表
void SignDevice::Init()
{
    static bool initflag = false;
    if(initflag)//确保只初始化一次
        return ;

//    QStringList fFileContent;//文件内容字符串列表
//    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("table.txt"));  //以文件方式读出
//    //如果配置文件不存在则写入默认文件
//    if(!aFile.exists()){
//        aFile.open(QIODevice::WriteOnly);
//        QTextStream aStream(&aFile); //用文本流读取文件
////        aStream << tr("id\t\t组名\t\t名称\t\t标示语\t\t网络状态\t\t语音设置\t\t闪光灯设置\t\t警示设置\t\t设备控制");
//        aStream << tr("id\t\t组名\t\t名称\t\t标示语\t\t网络状态\t\t语音设置\t\t闪光灯设置\t\t警示设置");
//        aFile.close();
//    }
//    if (aFile.open(QIODevice::ReadOnly | QIODevice::Text)) //以只读文本方式打开文件
//    {
//        QTextStream aStream(&aFile); //用文本流读取文件
//        while (!aStream.atEnd())
//        {
//            QString str=aStream.readLine();//读取文件的一行
//            fFileContent.append(str); //添加到 StringList
//        }
//        aFile.close();//关闭文件

//        for (int i = 1; i < fFileContent.count(); i++)//第一行是表头 跳过
//        {
//            SignDevice* signdev = new SignDevice();
//            signdev->deserialization(fFileContent.at(i));//获取数据区的一行 并进行反序列化
//            SignDeviceTable[signdev->id] = signdev;//添加到表中
//        }
//    }

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
    QJsonArray signDevArray = json["SignDeviceTable"].toArray();
    for (int idx = 0; idx < signDevArray.size(); ++idx)
    {
        QJsonObject signDevObj = signDevArray[idx].toObject();
        SignDevice* signdev = new SignDevice();
        signdev->deserialization(signDevObj);//获取数据区的一行 并进行反序列化
        SignDeviceTable[signdev->id] = signdev;//添加到表中
    }

    initflag = true;
}

//保存警示表
bool SignDevice::save()
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
    foreach(SignDevice* signdev, SignDeviceTable)
    {
        //序列化警示语对象 保存到文本中
        QJsonObject signDevObj;
        signdev->serialization(signDevObj);
        signDevArray.append(signDevObj);
    }
    tableObj["SignDeviceTable"] = signDevArray;
    QJsonDocument saveDoc(tableObj);
    saveFile.write(saveDoc.toJson());
//    saveFile.write(saveDoc.toBinaryData());
    saveFile.close();




//    //记录表头 确保不会被清空
//    QString tablehead;
//    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("table.txt"));  //以文件方式读出
//    if (aFile.open(QIODevice::ReadOnly | QIODevice::Text)) //以只读文本方式打开文件
//    {
//        QTextStream aStream(&aFile); //用文本流读取文件
//        tablehead = aStream.readLine();//第一行即为表头
//        aFile.close();//关闭文件
//    }else
//        return false;
//    //以读写、覆盖原有内容方式打开文件
//    //注意：会清空并打开文件
//    if (!(aFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)))
//        return false;

//    QTextStream aStream(&aFile); //用文本流读取文件

//    //保存表头
//    aStream << tablehead << "\n";  //文件里需要加入换行符 \n

//    //遍历整个警示语表
//    foreach(SignDevice* signdev, SignDeviceTable)
//        //序列化警示语对象 保存到文本中
//        aStream << signdev->serialization();
    return true;
}














////创建控件
//QWidget *signDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
//{
//    QComboBox *editor = new QComboBox(parent);

//    for(auto it = SignTable.begin(); it != SignTable.end(); ++it)
//        editor->addItem(SignTable[it.key()]);
//    editor->installEventFilter(const_cast<signDelegate*>(this));
////    connect(editor, &QComboBox::currentTextChanged, this, &signDelegate::currentTextChanged);
////    connect(editor, static_cast<void (QComboBox:: *)(int)>(&QComboBox::currentIndexChanged), this, &signDelegate::currentIndexChanged);

//    return editor;
//}
////设置控件显示的数据 将Model中的数据更新到Delegate中
//void signDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
//{
//    QString text =index.model()->data(index,Qt::DisplayRole).toString();
//    QComboBox *cmb = static_cast<QComboBox*>(editor);
//    cmb->setCurrentText(text);
//}
////将Delegate中的数据更新到Model中
//void signDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
//{
//    QComboBox *cmb = static_cast<QComboBox*>(editor);
//    QString text= cmb->currentText();
//    model->setData(index,text);
//}

//void signDelegate::currentTextChanged(const QString &str)
//{
////    if(QMessageBox::question(nullptr, tr("提示"), tr("是否更换标示语")) == QMessageBox::Yes)
////    {
////        //更换
////        qDebug() << "更换";
////    }else
////    {
////        //不更换
////        qDebug() << "不更换";
////    }
//}

//void signDelegate::currentIndexChanged(int idx)
//{
//    qDebug() << "更换";
////    if(QMessageBox::question(mainWindow, tr("提示"), tr("是否更换标示语")) == QMessageBox::Yes)
////    {
////        //更换
////        qDebug() << "更换";
////    }else
////    {
////        //不更换
////        qDebug() << "不更换";
////    }
//    //模态显示对话框
//    QMessageBox* msgBox = new QMessageBox( mainWindow );
//    msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
//    msgBox->setStandardButtons( QMessageBox::Ok );
//    msgBox->setWindowTitle(tr("提示"));
//    msgBox->setText(tr("是否更换标示语"));
//    msgBox->setModal( false ); // if you want it non-modal

//    msgBox->setIcon(QMessageBox::Question);
//    msgBox->show();
//}





//创建控件
QWidget *buttonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    qDebug() << "createEditor";
    QPushButton *editor = new QPushButton(tr("更换标示语"), parent);
    connect(editor, &QPushButton::clicked, this, [&](){
        qDebug() << "clicked";
    });
    return editor;
}
//设置控件显示的数据 将Model中的数据更新到Delegate中
void buttonDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    qDebug() << "setEditorData";
//    QString text =index.model()->data(index,Qt::DisplayRole).toString();
//    QPushButton *button = static_cast<QPushButton*>(editor);
//    button->setText(text);
}
//将Delegate中的数据更新到Model中
void buttonDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(editor);
    qDebug() << "setModelData";
//    QPushButton *button = static_cast<QPushButton*>(editor);
//    QString text= button->text();
//    model->setData(index,text);
}

























QFormMain::QFormMain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormMain)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除

    theModel = new QStandardItemModel(0, 0, this); //数据模型
    theSelection = new QItemSelectionModel(theModel);//选择模型

    ui->tableView->setModel(theModel); //设置数据模型
    ui->tableView->setSelectionModel(theSelection); //设置选择模型

    //设置代理
    buttonDelegate buttondelegate;
//    ui->tableView->setItemDelegateForColumn(3, &buttondelegate);

    //初始化配置文件
    QFormOptions::Init();
    //初始化标示语表
    Sign::Init();
    //初始化分组
    Group::Init();
    //初始化设备表
    SignDevice::Init();



    //加载表格
//    theModel->setHorizontalHeaderLabels({"id", "组名", "名称", "标示语", "网络状态", "语音设置", "闪光灯设置", "警示设置", "设备控制"}); //设置表头文字
    theModel->setHorizontalHeaderLabels({"id", "组名", "名称", "标示语", "网络状态", "语音状态", "闪光灯状态", "警示状态", "异常与故障状态"}); //设置表头文字
    foreach(SignDevice* signdev, SignDevice::getSignDevTable())
        signdev->item = addLine(signdev);
//        signdev->item = addLine(signdev->id, signdev->groupname, signdev->name, signdev->signid, signdev->voice==1, signdev->flash==1, signdev->alert==1);//添加一行到表格中

    //tcp server
    TcpServer& server = TcpServer::getHandle();
    //连接tcp信号槽
    connect(&server, static_cast<void (TcpServer:: *)(int, void*)>(&TcpServer::message),
            this, static_cast<void (QFormMain:: *)(int, void*)>(&QFormMain::recMessage));

    //安装过滤器 捕获搜索快捷键
    installEventFilter(this);
}

QFormMain::~QFormMain()
{
    delete ui;
}



//消息接收槽函数
void QFormMain::recMessage(int type, void* message)
{
    if(message == nullptr)
        return;

    switch(type)
    {
        case(MESSAGE_CHANGE_STATUS):
        {
            TcpDevice* tcpdev = static_cast<TcpDevice*>(message);
            SignDevice* signdev = SignDevice::findSignDev(tcpdev->id);
            if(signdev == nullptr)
            {
                qDebug() << "查找警示牌设备错误，id：" << tcpdev->id;
                return ;
            }
            *signdev = *tcpdev;//更新数据
            signdev->offline = 0;//在线
            refreshSignDev(signdev);
            qDebug() << "改变状态" << signdev->id << " : " << signdev->name << " : " << signdev->senderName;
            break;
        }
        case(MESSAGE_ADDDEVICE):
        {
            TcpDevice* tcpdev = static_cast<TcpDevice*>(message);
            //查看系统中是否已经存在
            SignDevice* signdev = SignDevice::findSignDev(tcpdev->id);
            if(signdev == nullptr)//新设备 插入表中
            {
                SignDevice* signdev = new SignDevice(*tcpdev);
                signdev->offline = 0;//在线
                //提醒用户 新设备上线
                MainWindow::showMessageBox(QMessageBox::Information, "通知", "新设备："+signdev->name+" 上线", 2000);

                addDevice(signdev);
                qDebug() << "添加新设备" << signdev->id << " : " << signdev->name << " : " << signdev->senderName;
            }else//老设备上线
            {
                *signdev = *tcpdev;//更新数据
                signdev->offline = 0;//在线
                refreshSignDev(signdev);
                qDebug() << "更新设备" << signdev->id << " : " << signdev->name << " : " << signdev->senderName;
            }
            break;
        }
        case(MESSAGE_DISCONNECTION):
        {
            TcpDevice* tcpdev = static_cast<TcpDevice*>(message);
            //查看系统中是否已经存在
            SignDevice* signdev = SignDevice::findSignDev(tcpdev->id);
            if(signdev == nullptr)//异常
            {
                QMessageBox::warning(this, "警告", "该设备"+tcpdev->id+"未在列表中");
            }else//断线
            {
                signdev->offline = 1;//断线
                refreshSignDev(signdev);
            }
            break;
        }
    }
}




//设置分组下拉框
void QFormMain::setGroupCombox(QComboBox *combobox)
{
    combobox->addItems(Group::getGroupNameList());
}

//设置警示语下拉框
void QFormMain::setSignCombox(QComboBox *combobox)
{
//    auto setCmbColor = [](QComboBox *combobox, const QColor color){
//        QPalette plt = combobox->palette();
//        plt.setColor(QPalette::Text, color);
//        combobox->setPalette(plt);
//    };

    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(combobox->model());
    int i = 0;
    foreach(Sign* sign, Sign::getSignTable())
    {
        combobox->addItem(sign->getIcon(), sign->text, QVariant::fromValue(sign));//添加图标 文字 自定义对象
        pItemModel->item(i++)->setForeground(sign->getColor());//修改某项文本颜色
    }

    //设置颜色
//    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(combobox->model());
//    for (int i = 0; i < combobox->count(); i++)
//    {
//        Sign* sign = combobox->itemData(i).value<Sign*>();
//        pItemModel->item(i)->setForeground(sign->getColor());//修改某项文本颜色
////        pItemModel->item(i)->setBackground(sign.getColor());//修改某项背景颜色
//    }
////    for (int i = 0; i < combobox->count(); i++) {
//////            pItemModel->item(i)->setIcon(QIcon(":/TestComboBox/Resources/deletered.png"));    //修改某项图标
//////            pItemModel->item(i)->setText("修改的文本  " + QString::number(i + 1));                          //修改某项文本
////        QString str = pItemModel->item(i)->text();
////        pItemModel->item(i)->setForeground(QColor(255, 0, 0));            //修改某项文本颜色
////        pItemModel->item(i)->setBackground(QColor(220,220,220));    //修改某项背景颜色
//////            pItemModel->item(i)->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);    //修改某项文本对齐方式
////    }
////    combobox->setStyleSheet("QComboBox{background:black}");
//    sign.color = combobox->currentData().toString().toUShort(nullptr, 16);
//    combobox->setStyleSheet(QString("QComboBox{background:%1}").arg(sign.getStrColor()));

    Sign* sign = combobox->currentData().value<Sign*>();
    combobox->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));
    connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        //先获取信号的发送者
        QComboBox *cmb = qobject_cast<QComboBox *>(sender());
        Sign* sign = cmb->itemData(index).value<Sign*>();
        cmb->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));

        //设置字体颜色
//        QPalette plt = cmb->palette();
//        static QColor color;
//        color = sign->getColor();
//        plt.setColor(QPalette::Text, QColor(150, 10, 55));
//        cmb->setPalette(plt);
    });
}

//设置开关选项下拉框
void QFormMain::setSwitchCombox(QComboBox *combobox)
{
    combobox->addItems({"关","开"});
}




//添加一行
QStandardItem *QFormMain::addLine(const SignDevice *signdev)
{
    //排除意外情况
    if(signdev == nullptr)
    {
        QMessageBox::warning(this, tr("警告"), "传入指针无效");
        return nullptr;
    }
    Sign* sign = Sign::findSign(signdev->signid);
    if(sign == nullptr)
    {
        QMessageBox::warning(this, tr("警告"), QString("未找到该标示语id：")+signdev->signid+", 插入设备错误");
        return nullptr;
    }
    //在表格最后添加行
    QList<QStandardItem*> aItemList; //容器类
    QStandardItem *aItem;
    QStandardItem *res;

    //id
    aItem=new QStandardItem(signdev->id);
    res = aItem;
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //组名
    aItem=new QStandardItem(signdev->groupname);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //标识牌名称
    aItem=new QStandardItem(signdev->name);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //标示语
    aItem=new QStandardItem(sign->text);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItem->setForeground(sign->getQtColor());//设置字体颜色
    aItem->setIcon(sign->getIcon());//设置图标
    aItem->setData(QVariant::fromValue((Sign *)sign));//存放标示语的指针
    aItemList << aItem;
    //网络状态（默认均离线）
    aItem=new QStandardItem();
    if(signdev->offline)
    {
        aItem->setText("离线");
        aItem->setIcon(QIcon(":/icon/lixian_1.png"));//设置图标
    }else
    {
        aItem->setText("在线");
        aItem->setIcon(QIcon(":/icon/zaixian_1.png"));//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //语音设置
    aItem=new QStandardItem();
    if(signdev->voice)
    {
        aItem->setText("开");
        aItem->setIcon(QIcon(":/icon/kaiguankai.png"));//设置图标
    }else
    {
        aItem->setText("关");
        aItem->setIcon(QIcon(":/icon/kaiguanguan.png"));//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //闪光灯设置
    aItem=new QStandardItem();
    if(signdev->flash)
    {
        aItem->setText("开");
        aItem->setIcon(QIcon(":/icon/kaiguankai.png"));//设置图标
    }else
    {
        aItem->setText("关");
        aItem->setIcon(QIcon(":/icon/kaiguanguan.png"));//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //警示设置
    aItem=new QStandardItem();
    if(signdev->alert)
    {
        aItem->setText("开");
        aItem->setIcon(QIcon(":/icon/kaiguankai.png"));//设置图标
    }else
    {
        aItem->setText("关");
        aItem->setIcon(QIcon(":/icon/kaiguanguan.png"));//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //报警状态
    QString sta;
    QBrush brush = QBrush(Qt::white);
    if(signdev->stafault)
    {
        if(signdev->fault.people_approach)
        {
            sta += "人员靠近 ";
            brush = QBrush(Qt::darkRed);
        }
        if(signdev->fault.Power)
        {
            sta += "电源故障 ";
            brush = QBrush(Qt::darkBlue);
        }
        if(signdev->fault.controller)
        {
            sta += "控制器故障 ";
            brush = QBrush(Qt::darkCyan);
        }
        if(signdev->fault.power_off)
        {
            sta += "交流电断电 ";
            brush = QBrush(Qt::darkMagenta);
        }
        if(signdev->fault.low_battery)
        {
            sta += "电池电量过低 ";
            brush = QBrush(Qt::darkYellow);
        }
        if(signdev->fault.manual_configuration)
        {
            sta += "红外遥控手动配置中 ";
            brush = QBrush(Qt::darkGray);
        }
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", signdev->name+":"+sta, 2000);
    }else
        sta = "无异常";
    aItem=new QStandardItem(sta);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
//    aItem->setForeground(brush);//设置文字颜色
//    theModel->setData(aItem->index(), brush, Qt::BackgroundColorRole);//设置颜色
    aItemList << aItem;




    //添加到新的一行
    theModel->insertRow(theModel->rowCount(), aItemList); //插入一行，需要每个Cell的Item
    theModel->setData(aItem->index(), brush, Qt::BackgroundColorRole);//设置状态栏颜色
    QModelIndex curIndex = theModel->index(theModel->rowCount()-1, 0);//创建最后一行的ModelIndex
    theSelection->clearSelection();//清空选择项
    theSelection->setCurrentIndex(curIndex, QItemSelectionModel::Select);//设置刚插入的行为当前选择行

    return res;
}

//添加一行（已经废弃）
QStandardItem* QFormMain::addLine(const QString& id, const QString& groupname, const QString& name, const QString& signid, bool voice, bool flash, bool alert)
{
    //排除意外情况
    Sign* sign = Sign::findSign(signid);
    if(sign == nullptr)
    {
        QMessageBox::warning(this, tr("警告"), QString("未找到该标示语id：")+signid+", 插入设备错误");
        return nullptr;
    }
//    SignDevice* signdev = SignDevice::findSignDev(id);
//    if(signdev == nullptr)
//    {
//        QMessageBox::warning(this, tr("警告"), QString("未找到该设备的id：")+signid+", 插入设备错误");
//        return ;
//    }
    //在表格最后添加行
    QList<QStandardItem*> aItemList; //容器类
    QStandardItem *aItem;
    QStandardItem *res;

    //id
    aItem=new QStandardItem(id);
    res = aItem;
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //组名（先占坑）
//    aItemList << new QStandardItem();
    aItem=new QStandardItem(groupname);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //标识牌名称
    aItem=new QStandardItem(name);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //标示语（先占坑）
//    aItemList << new QStandardItem();
    aItem=new QStandardItem(sign->text);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItem->setForeground(sign->getQtColor());//设置字体颜色
    aItem->setIcon(sign->getIcon());//设置图标
    aItem->setData(QVariant::fromValue((Sign *)sign));//存放标示语的指针
    aItemList << aItem;
    //网络状态（默认均离线）
    aItem=new QStandardItem("离线");
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //语音设置（先占坑）
//    aItemList << new QStandardItem();
    aItem=new QStandardItem(voice?tr("开"):tr("关"));
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //闪光灯设置（先占坑）
//    aItemList << new QStandardItem();
    aItem=new QStandardItem(flash?tr("开"):tr("关"));
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //警示设置（先占坑）
//    aItemList << new QStandardItem();
    aItem=new QStandardItem(alert?tr("开"):tr("关"));
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //设备控制
//    aItem=new QStandardItem("修改下位机");
//    QFont font = aItem->font();
//    font.setUnderline(true);
//    font.setItalic(true);
//    aItem->setFont(font);
//    aItem->setTextAlignment(Qt::AlignCenter);
//    aItem->setEditable(false);
//    aItemList << aItem;   //添加到容器

    //添加到新的一行
    theModel->insertRow(theModel->rowCount(), aItemList); //插入一行，需要每个Cell的Item
    QModelIndex curIndex = theModel->index(theModel->rowCount()-1, 0);//创建最后一行的ModelIndex
    theSelection->clearSelection();//清空选择项
    theSelection->setCurrentIndex(curIndex, QItemSelectionModel::Select);//设置刚插入的行为当前选择行

    return res;

    //组名
//    QComboBox* combobox = new QComboBox(this);
//    setGroupCombox(combobox);
//    combobox->setCurrentText(groupname);//设置对应的组名
//    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 1), combobox);
//    //标示语
//    combobox = new QComboBox(this);
//    setSignCombox(combobox);
//    combobox->setCurrentText(sign->text);//如果没有 默认设置为第一项
//    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 3), combobox);
//    //语音设置
//    combobox = new QComboBox(this);
//    setSwitchCombox(combobox);
//    combobox->setCurrentText(voice?tr("开"):tr("关"));//如果没有 默认设置为第一项
//    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 5), combobox);
//    //闪光灯设置
//    combobox = new QComboBox(this);
//    setSwitchCombox(combobox);
//    combobox->setCurrentText(flash?tr("开"):tr("关"));//如果没有 默认设置为第一项
//    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 6), combobox);
//    //警示设置
//    combobox = new QComboBox(this);
//    setSwitchCombox(combobox);
//    combobox->setCurrentText(alert?tr("开"):tr("关"));//如果没有 默认设置为第一项
//    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 7), combobox);
}

//更新 SignDevice 中的数据到表格中
QBitArray QFormMain::refreshSignDev(SignDevice *signdev)
{
    QBitArray res(16, 0);
    Sign *sign = Sign::findSign(signdev->signid);
    if(sign == nullptr)
    {
        QMessageBox::critical(this, "错误", "找不到对应的标示语id："+signdev->signid+" 刷新数据无效");
        return res;
    }
    if(signdev->item == nullptr)
    {
        QMessageBox::warning(this, "警告", "item指针无效，刷新数据无效");
        return res;
    }


    res[1] = modifyCell(signdev->item->row(), 1, signdev->groupname);
    res[2] = modifyCell(signdev->item->row(), 2, signdev->name);
    //设置警示语
    QStandardItem* signItem = theModel->item(signdev->item->row(), 3);//警示语栏
    Sign * lastsign = signItem->data().value<Sign*>();//取出上次的指针
    if(lastsign != sign)
    {
        //更新标示语
        res[3] = true;
        signItem->setForeground(sign->getColor());//设置字体颜色
        signItem->setIcon(sign->getIcon());//设置图标
        signItem->setText(sign->text);
        //修改自定义数据 放入最新指针
        signItem->setData(QVariant::fromValue((Sign *)sign));//存放标示语的指针
    }

    if(signdev->offline)
        res[4] = modifyCell(signdev->item->row(), 4, "离线", QIcon(":/icon/lixian_1.png"));
    else
        res[4] = modifyCell(signdev->item->row(), 4, "在线", QIcon(":/icon/zaixian_1.png"));
    if(signdev->voice)
        res[5] = modifyCell(signdev->item->row(), 5, "开", QIcon(":/icon/kaiguankai.png"));
    else
        res[5] = modifyCell(signdev->item->row(), 5, "关", QIcon(":/icon/kaiguanguan.png"));
    if(signdev->flash)
        res[6] = modifyCell(signdev->item->row(), 6, "开", QIcon(":/icon/kaiguankai.png"));
    else
        res[6] = modifyCell(signdev->item->row(), 6, "关", QIcon(":/icon/kaiguanguan.png"));
    if(signdev->alert)
        res[7] = modifyCell(signdev->item->row(), 7, "开", QIcon(":/icon/kaiguankai.png"));
    else
        res[7] = modifyCell(signdev->item->row(), 7, "关", QIcon(":/icon/kaiguanguan.png"));

    //设置报警状态
    QString sta;
    QBrush brush = QBrush(Qt::white);
    if(signdev->stafault)
    {
        if(signdev->fault.people_approach)
        {
            sta += "人员靠近 ";
            brush = QBrush(Qt::darkRed);
        }
        if(signdev->fault.Power)
        {
            sta += "电源故障 ";
            brush = QBrush(Qt::darkBlue);
        }
        if(signdev->fault.controller)
        {
            sta += "控制器故障 ";
            brush = QBrush(Qt::darkCyan);
        }
        if(signdev->fault.power_off)
        {
            sta += "交流电断电 ";
            brush = QBrush(Qt::darkMagenta);
        }
        if(signdev->fault.low_battery)
        {
            sta += "电池电量过低 ";
            brush = QBrush(Qt::darkYellow);
        }
        if(signdev->fault.manual_configuration)
        {
            sta += "红外遥控手动配置中 ";
            brush = QBrush(Qt::darkGray);
        }
    }else
        sta = "无异常";
    res[8] = modifyCell(signdev->item->row(), 8, sta, QIcon(), brush);
    if(res[8] && signdev->stafault)//如果修改了 且 有异常 则需要提醒
    {
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", signdev->name+":"+sta, 2000);
    }
    return res;
}


//事件截获
bool QFormMain::eventFilter(QObject *watched, QEvent *event)
{
    if(nullptr != event && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if((keyEvent->key() == Qt::Key_F) && (keyEvent->modifiers() == Qt::ControlModifier))//Ctrl+F 搜索
        {
//            qDebug() << "Ctrl+F";
            bool ok=false;
            QString searchname = QInputDialog::getText(this, tr("搜索标示语名称"),
                                                 tr("请输入待搜索的标示语名称："), QLineEdit::Normal,
                                                 QString(""), &ok);
            //只有当用户输入不为空且不在列表中存在时才插入
            if (ok && !searchname.isEmpty())
            {
                bool sta = false;
                foreach(SignDevice* signdev, SignDevice::getSignDevTable())
                    if(signdev->name == searchname)
                    {
                        sta = true;
                        ui->tableView->selectRow(signdev->item->row());//光标选中该行
//                        theSelection->setCurrentIndex(signdev->item->index(), QItemSelectionModel::Select);
                        break;
                    }
                if(!sta)
                    QMessageBox::information(this, "通知", "没有找到警示牌："+searchname);
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

//关闭窗口信号
void QFormMain::closeEvent(QCloseEvent *event)
{
    QMessageBox::warning(this, "警告", "该窗口禁止关闭");
    event->ignore();  //忽略退出信号，程序继续运行
}





//添加设备
void QFormMain::on_pushButton_AddDevice_clicked()
{
    //弹窗
    QDialogAddDev *dlgAddDev = new QDialogAddDev(this);
    int ret = dlgAddDev->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)//OK按钮被按下
    {
        if(dlgAddDev->getDeviceId().isEmpty())
        {
//            QMessageBox::warning(this, "警告", "添加失败（id为空），请连接设备");
            QMessageBox::warning(this, "警告", "添加失败（id为空），请输入id");
            delete dlgAddDev;
            return ;
        }
        SignDevice* signdev = new SignDevice(dlgAddDev->getDeviceId(), dlgAddDev->getGroupName(), dlgAddDev->getName(), dlgAddDev->getSignIdx());
        addDevice(signdev);
    }
    delete dlgAddDev;
}

//删除设备
void QFormMain::on_pushButton_DelDevice_clicked()
{
    //删除行
    QModelIndex curIndex=theSelection->currentIndex();//获取模型索引
    if(!curIndex.isValid())
    {
        QMessageBox::information(this, "通知", "列表为空，不能再删除");
        return ;
    }
    int counts = theModel->rowCount();
    delDevice(curIndex.row());

    //如果不是删除最后一行 则重新设置当前选择行
    if(curIndex.row () != counts - 1)
        theSelection->setCurrentIndex (curIndex, QItemSelectionModel::Select);
}

//保存设备列表
void QFormMain::on_pushButton_SaveTable_clicked()
{
    //确保id唯一
    QSet<QString> _set;
    for (int i = 0; i < theModel->rowCount(); i++)
    {
        QString id = theModel->item(i,0)->text();
        _set.insert(id);
    }
    if(_set.size() != theModel->rowCount())
    {
        QMessageBox::warning(this, tr("警告"), tr("保存设备列表失败（请确保id唯一）"));
        return;
    }

    //保存数据
    SignDevice::save();

    QMessageBox::information(this, tr("通知"), tr("保存设备列表成功"));
}

//标示语表
void QFormMain::on_pushButton_SignTable_clicked()
{
    QFormSignTable *formSignTable = new QFormSignTable(this);
    formSignTable->setWindowFlag(Qt::Window,true);
    formSignTable->setWindowModality(Qt::WindowModal);//窗口阻塞，但是该函数不会阻塞
    formSignTable->show();
}





void QFormMain::on_tableView_clicked(const QModelIndex &index)
{
//    QAbstractItemModel *Imodel=ui->tableView->model();
    //更换标示语
//    if(index.column() == 8)
//    {
//        if(QMessageBox::question(mainWindow, tr("提示"), tr("是否更换标示语")) == QMessageBox::Yes)
//        {
//            //更换
//        }else
//        {
//            //不更换
//        }
//    }
//    QModelIndex Iindex = Imodel->index(index.row(),1);//index.row()为算选择的行号。1为所选中行的第一列。。
//    QVariant datatemp=Imodel->data(Iindex);
//    QString name=datatemp.toString();//name即为所选择行的第一列的值。。。
}



//双击修改对象
void QFormMain::on_tableView_doubleClicked(const QModelIndex &index)
{
    //获得该行对应的对象
    QString id = theModel->data(theModel->index(index.row(), 0)).toString();//获得该行的id
    SignDevice* signdev = SignDevice::findSignDev(id);
    //弹窗
    QDialogSetSignDev *dlgSignDev = new QDialogSetSignDev(this);
    dlgSignDev->setParameters(signdev);//预先设置
    int ret = dlgSignDev->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)//OK按钮被按下
    {
        //刷新表格数据
        QBitArray res = refreshSignDev(signdev);
        //修改相关设置 需要发送给下位机
        for(int i = 1; i < res.size(); i++)//第0项为id 不可能修改
        {
            if(res[i])//该项有改动
            {
                //todo 发送信息给下位机 通知设置已经更改
                //todo 这里只记录了三个check状态 其他改变未记录
                qDebug() << QString("第")+i+"项有改动";

            }
        }
        //有修改 同时下位机在线 则给下位机发送数据
        //注意：这里会统计所有有修改的数量，但是该处由于 QDialogSetSignDev 的限制 能修改的只有
        //      名字、标示语、三个开关量，而这些改变均需要通知下位机。其他如id、是否在线、报警状态，
        //      在这里则不可能修改。所以用 res.count 是没问题的
        if(res.count(true) != 0 && signdev->offline == 0)
        {
            Sign *sign = Sign::findSign(signdev->signid);
            //发送信息
            TcpServer& server = TcpServer::getHandle();
            TcpDevice* tcpdev = server.findTcpDevice(signdev->id);
            QByteArray data;
            data += signdev->stabyte;//模式，即状态字
            data += sign->color;//颜色
            data += tcpdev->light;//亮度
            data += tcpdev->vol;//音量
            data += tcpdev->delay;//报警时间
            data += QString("%1").arg(sign->id, 3, QLatin1Char('0')).toLocal8Bit();//提示语编号
            data += "000";//图片编号
            server.sendMessage(signdev->id, 00, 02, data);//发送状态信息
            data.clear();

            if(res[2])//修改名称
            {
                data += QString("%1").arg(signdev->name, -16, QLatin1Char(0)).toLocal8Bit();//名称
                data += QString("%1").arg("", -16, QLatin1Char(0)).toLocal8Bit();//安装位置
                server.sendMessage(signdev->id, 00, 71, data);//发送警示牌名称
                data.clear();
            }

            if(res[3])//只有当警示语变更时才发送文本信息
            {
                data += QString("%1").arg(sign->id, 3, QLatin1Char('0')).toLocal8Bit();//提示语编号
                data += sign->text.toLocal8Bit();//警示语内容
                server.sendMessage(signdev->id, 00, 12, data);//发送警示语文本信息
                data.clear();
            }
        }
    }
    delete dlgSignDev;
}

