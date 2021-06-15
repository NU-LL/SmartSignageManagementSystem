#include "qformmain.h"
#include "ui_qformmain.h"

#include "config.h"


#include <QDebug>
#include <QStandardItemModel>
#include <QItemSelectionModel>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QMessageBox>


#include "protocol.h"

#include "qformsigntable.h"
#include "qdialogadddev.h"

QFormMain::QFormMain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormMain)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除


    theModel = new QStandardItemModel(2, TABLE_COLUMS, this); //数据模型
    theSelection = new QItemSelectionModel(theModel);//选择模型

    ui->tableView->setModel(theModel); //设置数据模型
    ui->tableView->setSelectionModel(theSelection); //设置选择模型


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
}

QFormMain::~QFormMain()
{
    delete ui;
}




void QFormMain::iniModelFromStringList(QStringList& aFileContent)
{ //从一个StringList 获取数据，初始化数据Model
    int rowCnt=aFileContent.count(); //文本行数，第1行是标题
    theModel->setRowCount(rowCnt-1); //实际数据行数
    //设置表头
    QString header=aFileContent.at(0);//第1行是表头
    //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
    QStringList headerList = header.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    theModel->setHorizontalHeaderLabels(headerList); //设置表头文字

    //设置表格数据
    QStringList tmpList;
    int j;
    QStandardItem   *aItem;
    for (int i=1;i<rowCnt;i++)
    {
        int j = 0;
        //提取文字
        QString aLineText = aFileContent.at(i); //获取数据区的一行
        //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
        QStringList tmpList = aLineText.split(QRegExp("\\s+"),QString::SkipEmptyParts);


        //第一列是 Checkable
        aItem=new QStandardItem();
        aItem->setCheckable(true); //设置为Checkable
//        aItem->setSizeHint(QSize(10,10));
        aItem->setTextAlignment(Qt::AlignCenter);
        if (tmpList.at(j) == "0")
            aItem->setCheckState(Qt::Unchecked); //根据数据设置check状态
        else
            aItem->setCheckState(Qt::Checked);
        theModel->setItem(i-1,j,aItem); //为模型的某个行列位置设置Item
        //剩下列
        for (j = 1; j < TABLE_COLUMS; j++) //tmpList的行数等于FixedColumnCount, 固定的
        {
            //不包含最后一列
            aItem=new QStandardItem(tmpList.at(j));//创建item
            aItem->setTextAlignment(Qt::AlignCenter);
            aItem->setEditable(false);
            theModel->setItem(i-1,j,aItem); //为模型的某个行列位置设置Item
        }
    }
}

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

        for(j = 1; j < theModel->columnCount(); j++)
        {
            aItem = theModel->item(i,j);
            str = str+aItem->text() + QString::asprintf("\t\t");
        }

        aStream << str << "\n";
    }
}

//加载 SignTable
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
        //在表格最后添加行
        QList<QStandardItem*> aItemList; //容器类
        QStandardItem *aItem;

        //第一列
        aItem=new QStandardItem();
        aItem->setTextAlignment(Qt::AlignCenter);
        aItem->setCheckable(true);
        aItemList<<aItem;   //添加到容器
        //后三列
        aItem=new QStandardItem(dlgAddDev->getDevice()); //编号
        aItem->setTextAlignment(Qt::AlignCenter);
        aItem->setEditable(false);
        aItemList<<aItem;   //添加到容器
        aItem=new QStandardItem(dlgAddDev->getName()); //名称
        aItem->setTextAlignment(Qt::AlignCenter);
        aItem->setEditable(false);
        aItemList<<aItem;   //添加到容器
        aItem=new QStandardItem(dlgAddDev->getSign()); //标示语
        aItem->setTextAlignment(Qt::AlignCenter);
        aItem->setEditable(false);
        aItemList<<aItem;   //添加到容器
        //状态列
        for(int i = 4; i < theModel->columnCount(); i++) //不包含最后1列
        {
            aItem=new QStandardItem("0"); //创建Item
            aItem->setTextAlignment(Qt::AlignCenter);
            aItem->setEditable(false);
            aItemList<<aItem;   //添加到容器
        }

        theModel->insertRow(theModel->rowCount(), aItemList); //插入一行，需要每个Cell的Item
        QModelIndex curIndex = theModel->index(theModel->rowCount()-1, 0);//创建最后一行的ModelIndex
        theSelection->clearSelection();//清空选择项
        theSelection->setCurrentIndex(curIndex, QItemSelectionModel::Select);//设置刚插入的行为当前选择行

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



