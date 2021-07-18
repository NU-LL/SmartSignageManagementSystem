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
#include <QMessageBox>
#include <QComboBox>

#include "protocol.h"
#include "mainwindow.h"

#include "qformsigntable.h"
#include "qdialogadddev.h"
#include "qdialogsetsigndev.h"

extern MainWindow* mainWindow;


QMap<QString, SignDevice*> SignDevice::SignDeviceTable;


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
    QFile loadFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("table.json"));
    if(!loadFile.exists())
    {
        loadFile.open(QIODevice::WriteOnly);
        loadFile.close();
    }
    if (!loadFile.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open load file.");
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
    QFile saveFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("table.json"));
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        qWarning("Couldn't open save file.");
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

    //初始化标示语表
    Sign::Init();
    //初始化分组
    Group::Init();
    //初始化设备表
    SignDevice::Init();



    //加载表格
//    theModel->setHorizontalHeaderLabels({"id", "组名", "名称", "标示语", "网络状态", "语音设置", "闪光灯设置", "警示设置", "设备控制"}); //设置表头文字
    theModel->setHorizontalHeaderLabels({"id", "组名", "名称", "标示语", "网络状态", "语音状态", "闪光灯状态", "警示状态"}); //设置表头文字
    foreach(SignDevice* signdev, SignDevice::getSignDevTable())
        addLine(signdev->id, signdev->groupname, signdev->name, signdev->signid, signdev->voice==1, signdev->flash==1, signdev->alert==1);//添加一行到表格中

    //tcp server
    TcpServer& server = TcpServer::getHandle();
    //连接tcp信号槽
    connect(&server, static_cast<void (TcpServer:: *)(int, QString, QString, int, void*)>(&TcpServer::message),
            this, static_cast<void (QFormMain:: *)(int, QString, QString, int, void*)>(&QFormMain::recMessage));
}

QFormMain::~QFormMain()
{
    delete ui;
}

//消息接收槽函数
void QFormMain::recMessage(int level, QString title, QString text, int message_id, void* message)
{
    if(message_id == MESSAGE_BOX)//否则下面对map的操作会有干扰，插入新的元素进入
        return ;

    TcpServer& server = TcpServer::getHandle();
    QString info = title + ":" + text;
    QString id = server.findId(info);
    TcpDevice* device = server.findTcpDevice(id);

    //数据有效性检查
    if(id == TcpServer::ERROR || device == nullptr)
        return ;


    switch(message_id)
    {
        case(MESSAGE_CHANGE_STATUS):
        {
            //更新所有状态
            QList<QStandardItem *> list = theModel->findItems(id, Qt::MatchFixedString, 1);//在第一列中查找id
            for(QStandardItem *aItem : qAsConst(list))
            {
                int row = aItem->row();
//                setStatus(row, device->stabyte);
                qDebug() << "修改行：" << row;
            }
            break;
        }
        case(MESSAGE_ADDDEVICE):
        {
//            //插入一行
//            QString sign;
//            if(SignTable.find(device->signidx) != SignTable.end())
//                sign = SignTable[device->signidx];
//            else
//            {
//                QMessageBox::warning(this, "警告", QString("找不到标示语编号：")+device->signidx);
//                qDebug() << "找不到标示语编号：" << device->signidx;
//            }
//            SignDevice* signdev = new SignDevice(signdev->id, signdev->groupname, signdev->name, signdev->signid);
//            addDevice(signdev);
//            SignDevice *signdev = new SignDevice;
//            addDevice(signdev->id, signdev->groupname, signdev->name, signdev->signid);
//            addDevice(&signdev);
//            addDevice(id, device->name, sign, device->stabyte);
            break;
        }
        case(MESSAGE_DISCONNECTION):
        {
            //删除一行
            QList<QStandardItem *> list = theModel->findItems(id, Qt::MatchFixedString, 1);//在第一列中查找id
            for(QStandardItem *aItem : qAsConst(list))
            {
                int row = aItem->row();
                qDebug() << "删除行：" << row;
                theModel->removeRow(row);
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
void QFormMain::addLine(const QString& id, const QString& groupname, const QString& name, const QString& signid, bool voice, bool flash, bool alert)
{
    //排除意外情况
    Sign* sign = Sign::findSign(signid);
    if(sign == nullptr)
    {
        QMessageBox::warning(this, tr("警告"), QString("未找到该标示语id：")+signid+", 插入设备错误");
        return ;
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

    //id
    aItem=new QStandardItem(id);
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



////设置状态
//void QFormMain::setStatus(int row, quint8 staBytes)
//{
//    //警示状态
//    if(staBytes&0x01)
//        modifyCell(row, 7, "开");
//    else
//        modifyCell(row, 7, "关");
//    //闪光状态
//    if(staBytes&0x02)
//        modifyCell(row, 6, "开");
//    else
//        modifyCell(row, 6, "关");
//    //语音状态
//    if(staBytes&0x04)
//        modifyCell(row, 5, "开");
//    else
//        modifyCell(row, 5, "关");
//    //离线状态
//    if(staBytes&0x08)
//        modifyCell(row, 4, "开");
//    else
//        modifyCell(row, 4, "关");
//}





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
            QMessageBox::warning(this, "警告", "添加失败（id为空），请连接设备");
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
        bool ret = false;
        //刷新表格数据
        modifyCell(index.row(), 1, signdev->groupname);
        modifyCell(index.row(), 2, signdev->name);
        //设置警示语
        {
            Sign *sign = Sign::findSign(signdev->signid);
            if(sign == nullptr)
            {
                QMessageBox::critical(this, "错误", "找不到对应的标示语id："+signdev->signid);
                delete dlgSignDev;
                return ;
            }
            QStandardItem* signItem = theModel->item(index.row(), 3);//警示语栏
            signItem->setForeground(sign->getColor());//设置字体颜色
            signItem->setIcon(sign->getIcon());//设置图标
            signItem->setText(sign->text);
        }
        ret |= modifyCell(index.row(), 5, signdev->TcpDevice::voice==1?"开":"关");
        ret |= modifyCell(index.row(), 6, signdev->TcpDevice::flash==1?"开":"关");
        ret |= modifyCell(index.row(), 7, signdev->TcpDevice::alert==1?"开":"关");
        //修改相关设置 需要发送给下位机
        if(ret)
        {
            //todo 发送信息给下位机 通知设置已经更改
            //todo 这里只记录了三个check状态 其他改变未记录
        }
    }
    delete dlgSignDev;
}

