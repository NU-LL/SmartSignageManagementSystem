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
#include <QRegExp>
#include <QMessageBox>
#include <QComboBox>

#include "protocol.h"
#include "mainwindow.h"

#include "qformsigntable.h"
#include "qdialogadddev.h"

extern MainWindow* mainWindow;


//创建控件
QWidget *signDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QComboBox *editor = new QComboBox(parent);
    for(auto it = SignTable.begin(); it != SignTable.end(); ++it)
        editor->addItem(SignTable[it.key()]);
    editor->installEventFilter(const_cast<signDelegate*>(this));
//    connect(editor, &QComboBox::currentTextChanged, this, &signDelegate::currentTextChanged);
//    connect(editor, static_cast<void (QComboBox:: *)(int)>(&QComboBox::currentIndexChanged), this, &signDelegate::currentIndexChanged);

    return editor;
}
//设置控件显示的数据 将Model中的数据更新到Delegate中
void signDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString text =index.model()->data(index,Qt::DisplayRole).toString();
    QComboBox *cmb = static_cast<QComboBox*>(editor);
    cmb->setCurrentText(text);
}
//将Delegate中的数据更新到Model中
void signDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cmb = static_cast<QComboBox*>(editor);
    QString text= cmb->currentText();
    model->setData(index,text);
}

void signDelegate::currentTextChanged(const QString &str)
{
//    if(QMessageBox::question(nullptr, tr("提示"), tr("是否更换标示语")) == QMessageBox::Yes)
//    {
//        //更换
//        qDebug() << "更换";
//    }else
//    {
//        //不更换
//        qDebug() << "不更换";
//    }
}

void signDelegate::currentIndexChanged(int idx)
{
    qDebug() << "更换";
//    if(QMessageBox::question(mainWindow, tr("提示"), tr("是否更换标示语")) == QMessageBox::Yes)
//    {
//        //更换
//        qDebug() << "更换";
//    }else
//    {
//        //不更换
//        qDebug() << "不更换";
//    }
    //模态显示对话框
    QMessageBox* msgBox = new QMessageBox( mainWindow );
    msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
    msgBox->setStandardButtons( QMessageBox::Ok );
    msgBox->setWindowTitle(tr("提示"));
    msgBox->setText(tr("是否更换标示语"));
    msgBox->setModal( false ); // if you want it non-modal

    msgBox->setIcon(QMessageBox::Question);
    msgBox->show();
}





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

    theModel = new QStandardItemModel(0, TABLE_COLUMS, this); //数据模型
    theSelection = new QItemSelectionModel(theModel);//选择模型

    ui->tableView->setModel(theModel); //设置数据模型
    ui->tableView->setSelectionModel(theSelection); //设置选择模型

//    buttonDelegate *bd = new buttonDelegate(this);
//    ui->tableView->setItemDelegateForColumn(1, bd);

    signDelegate *sd = new signDelegate(SignTable, this);
    ui->tableView->setItemDelegateForColumn(3, sd);

    //构建表格
    QStringList fFileContent;//文件内容字符串列表
    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("table.txt"));  //以文件方式读出
    //如果配置文件不存在则写入默认文件
    if(!aFile.exists()){
        aFile.open(QIODevice::WriteOnly);
        QTextStream aStream(&aFile); //用文本流读取文件
        aStream << tr("/\t\t编号\t\t名称\t\t标示语\t\t网络状态\t\t语音状态\t\t闪光灯状态\t\t警示状态\t\t设备控制");
        aFile.close();
    }
    if (aFile.open(QIODevice::ReadOnly | QIODevice::Text)) //以只读文本方式打开文件
    {
        QTextStream aStream(&aFile); //用文本流读取文件
        while (!aStream.atEnd())
        {
            QString str=aStream.readLine();//读取文件的一行
            fFileContent.append(str); //添加到 StringList
        }
        aFile.close();//关闭文件
        iniModelFromStringList(fFileContent);//从StringList的内容初始化数据模型
    }



    //加载SignTable
    loadSignTable();

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
    QString id;
    TcpDevice* device = nullptr;
    if(server.Ip2IdTable.find(info) != server.Id2IpTable.end())
        id = server.Ip2IdTable[info];
    if(!id.isEmpty() && server.DeviceTab.find(id) != server.DeviceTab.end())
        device = server.DeviceTab[id];
    else
        return ;//此时device为空


    switch(message_id)
    {
        case(MESSAGE_CHANGE_STATUS):
        {
            //更新所有状态
            QList<QStandardItem *> list = theModel->findItems(id, Qt::MatchFixedString, 1);//在第一列中查找id
            for(QStandardItem *aItem : qAsConst(list))
            {
                int row = aItem->row();
                setStatus(row, device->stabyte);
                qDebug() << "修改行：" << row;
            }
            break;
        }
        case(MESSAGE_ADDDEVICE):
        {
            //插入一行
            QString sign;
            if(SignTable.find(device->signidx) != SignTable.end())
                sign = SignTable[device->signidx];
            else
            {
                QMessageBox::warning(this, "警告", QString("找不到标示语编号：")+device->signidx);
                qDebug() << "找不到标示语编号：" << device->signidx;
            }
            addDevice(id, device->name, sign, device->stabyte);
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



//设置状态
void QFormMain::setStatus(int row, quint8 staBytes)
{
    //警示状态
    if(staBytes&0x01)
        modifyCell(row, 7, "开");
    else
        modifyCell(row, 7, "关");
    //闪光状态
    if(staBytes&0x02)
        modifyCell(row, 6, "开");
    else
        modifyCell(row, 6, "关");
    //语音状态
    if(staBytes&0x04)
        modifyCell(row, 5, "开");
    else
        modifyCell(row, 5, "关");
    //离线状态
    if(staBytes&0x08)
        modifyCell(row, 4, "开");
    else
        modifyCell(row, 4, "关");
}

//添加一行
void QFormMain::addDevice(const QString &id, const QString &name, const QString &Sign, quint8 staBytes)
{
    //在表格最后添加行
    QList<QStandardItem*> aItemList; //容器类
    QStandardItem *aItem;

    //第一列
    aItem=new QStandardItem();
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setCheckable(false);
    aItemList<<aItem;   //添加到容器
    //后三列
    aItem=new QStandardItem(id); //编号
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);
    aItemList<<aItem;   //添加到容器
    aItem=new QStandardItem(name); //名称
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);
    aItemList<<aItem;   //添加到容器
    aItem=new QStandardItem(Sign); //标示语
    aItem->setTextAlignment(Qt::AlignCenter);
//    aItem->setEditable(false);
    aItemList<<aItem;   //添加到容器
    //状态列
    for(int i = 4; i < theModel->columnCount()-1; i++) //不包含最后1列
    {
        aItem=new QStandardItem("关"); //创建Item
        aItem->setTextAlignment(Qt::AlignCenter);
        aItem->setEditable(false);
        aItemList<<aItem;   //添加到容器
    }
    //最后一列
    aItem=new QStandardItem("更换标示语"); //名称
    QFont font = aItem->font();
    font.setUnderline(true);
    font.setItalic(true);
    aItem->setFont(font);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);
    aItemList<<aItem;   //添加到容器



    theModel->insertRow(theModel->rowCount(), aItemList); //插入一行，需要每个Cell的Item
    QModelIndex curIndex = theModel->index(theModel->rowCount()-1, 0);//创建最后一行的ModelIndex
    theSelection->clearSelection();//清空选择项
    theSelection->setCurrentIndex(curIndex, QItemSelectionModel::Select);//设置刚插入的行为当前选择行

    //设置状态列
    setStatus(theModel->rowCount()-1, staBytes);
    //勾选列
    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 0), new QCheckBox(this));
//    //设备控制列
    //    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, theModel->columnCount()-1), button);
}





//关闭窗口信号
void QFormMain::closeEvent(QCloseEvent *event)
{
    QMessageBox::warning(this, "警告", "该窗口禁止关闭");
    event->ignore();  //忽略退出信号，程序继续运行
}


//从QStringList中构建表格
void QFormMain::iniModelFromStringList(QStringList& aFileContent)
{ //从一个StringList 获取数据，初始化数据Model
    int rowCnt=aFileContent.count(); //文本行数，第1行是标题
//    theModel->setRowCount(rowCnt-1); //实际数据行数
    //设置表头
    QString header=aFileContent.at(0);//第1行是表头
    //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
    QStringList headerList = header.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    theModel->setHorizontalHeaderLabels(headerList); //设置表头文字

    //设置表格数据
    QStringList tmpList;
    for (int i=1;i<rowCnt;i++)
    {
        int j = 0;
        //提取文字
        QString aLineText = aFileContent.at(i); //获取数据区的一行
        //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
        QStringList tmpList = aLineText.split(QRegExp("\\s+"),QString::SkipEmptyParts);

        addDevice(tmpList[0], tmpList[1], tmpList[2], tmpList[3] == "开", tmpList[4] == "开", tmpList[5] == "开", tmpList[6] == "开");
    }
}

//保存表格
void QFormMain::saveTable()
{
    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("table.txt"));
    if (!(aFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)))
        return; //以读写、覆盖原有内容方式打开文件

    QTextStream aStream(&aFile); //用文本流读取文件

    QStandardItem   *aItem;
    int i,j;
    QString str;

    //获取表头文字
    for (i=0;i<theModel->columnCount();i++)
    {
        aItem=theModel->horizontalHeaderItem(i); //获取表头的项数据
        str=str+aItem->text()+"\t\t";  //以TAB见隔开
    }
    aStream<<str<<"\n";  //文件里需要加入换行符 \n

    //获取数据区文字
    for ( i=0;i<theModel->rowCount();i++)
    {
        str = "";

        aItem = theModel->item(i,0); //第一列是逻辑型
        if (aItem->checkState() == Qt::Checked)
            str = str + "1" + "\t\t";
        else
            str = str + "0" + "\t\t";
        //最后一列是控件 无需保存
        for(j = 1; j < theModel->columnCount()-1; j++)
        {
            aItem = theModel->item(i,j);
            str = str+aItem->text() + QString::asprintf("\t\t");
        }

        aStream << str << "\n";
    }
}

//加载警示语表 SignTable
void QFormMain::loadSignTable()
{
    QStringList fFileContent;//文件内容字符串列表
    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("signtable.txt"));  //以文件方式读出
    //如果配置文件不存在则写入默认文件
    if(!aFile.exists()){
        aFile.open(QIODevice::WriteOnly);
        QTextStream aStream(&aFile); //用文本流读取文件
        aStream << tr("/\t\t索引\t\t标示语");
        aFile.close();
    }
    if(aFile.open(QIODevice::ReadOnly | QIODevice::Text)) //以只读文本方式打开文件
    {
        QTextStream aStream(&aFile); //用文本流读取文件
        while (!aStream.atEnd())
        {
            QString str=aStream.readLine();//读取文件的一行
            fFileContent.append(str); //添加到 StringList
        }
        aFile.close();//关闭文件

        SignTable.clear();
        //设置表格
        for (int i = 1; i < fFileContent.count(); i++)
        {
            //提取文字
            QString aLineText = fFileContent.at(i); //获取数据区的一行
            //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
            QStringList tmpList = aLineText.split(QRegExp("\\s+"),QString::SkipEmptyParts);

            //获得目标键值对
            int key = tmpList.at(1).toInt();
            SignTable[key] = tmpList.at(2);
        }
    }
}

//添加设备
void QFormMain::on_pushButton_AddDevice_clicked()
{
    //加载SignTable
    loadSignTable();

    //弹窗
    QDialogAddDev *dlgAddDev = new QDialogAddDev(this);
    int ret = dlgAddDev->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)
    {
        //OK按钮被按下
        addDevice(dlgAddDev->getDeviceId(), dlgAddDev->getName(), dlgAddDev->getSign(), 0);
    }
    delete dlgAddDev;
}

//删除设备
void QFormMain::on_pushButton_DelDevice_clicked()
{
    //删除行
    QModelIndex curIndex=theSelection->currentIndex() ;//获取模型索引
//    QModelIndexList selectedIndex=theSelection->selectedIndexes();//多选
    if (curIndex.row () ==theModel->rowCount () -1) //最后一行
        theModel->removeRow (curIndex.row () ) ; //删除最后一行
    else {
        theModel->removeRow (curIndex.row () );//删除一行，并重新设置当前选择行
        theSelection->setCurrentIndex (curIndex, QItemSelectionModel::Select);
    }
}

//保存设备列表
void QFormMain::on_pushButton_SaveTable_clicked()
{
    saveTable();
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
    if(index.column() == 8)
    {
        if(QMessageBox::question(mainWindow, tr("提示"), tr("是否更换标示语")) == QMessageBox::Yes)
        {
            //更换
        }else
        {
            //不更换
        }
    }
//    QModelIndex Iindex = Imodel->index(index.row(),1);//index.row()为算选择的行号。1为所选中行的第一列。。
//    QVariant datatemp=Imodel->data(Iindex);
//    QString name=datatemp.toString();//name即为所选择行的第一列的值。。。
}

