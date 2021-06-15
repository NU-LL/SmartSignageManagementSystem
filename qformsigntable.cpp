#include "qformsigntable.h"
#include "ui_qformsigntable.h"

#include "config.h"

#include <QStandardItemModel>
#include <QItemSelectionModel>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegExp>

#include "qformmain.h"

QFormSignTable::QFormSignTable(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormSignTable)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除


    theModel = new QStandardItemModel(2, SIGNTABLE_COLUMS, this); //数据模型
    theSelection = new QItemSelectionModel(theModel);//选择模型

    ui->tableView->setModel(theModel); //设置数据模型
    ui->tableView->setSelectionModel(theSelection); //设置选择模型
    ui->tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectItems);


    //构建模型
    QStringList fFileContent;//文件内容字符串列表
    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("signtable.txt"));  //以文件方式读出
    //如果配置文件不存在则写入默认文件
    if(!aFile.exists()){
        aFile.open(QIODevice::WriteOnly);
        QTextStream aStream(&aFile); //用文本流读取文件
        aStream << tr("/\t\t索引\t\t标示语");
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
}

void QFormSignTable::iniModelFromStringList(QStringList& aFileContent)
{
    //从一个StringList 获取数据，初始化数据Model
    int rowCnt=aFileContent.count(); //文本行数，第1行是标题
    theModel->setRowCount(rowCnt-1); //实际数据行数
    //设置表头
    QString header=aFileContent.at(0);//第1行是表头
    //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
    QStringList headerList = header.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    theModel->setHorizontalHeaderLabels(headerList); //设置表头文字

    //设置表格数据
    QStringList tmpList;

    QStandardItem   *aItem;
    for (int i = 1; i < rowCnt; i++)
    {
        int j = 0;
        //提取文字
        QString aLineText = aFileContent.at(i); //获取数据区的一行
        //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
        QStringList tmpList = aLineText.split(QRegExp("\\s+"),QString::SkipEmptyParts);


        //第一列是 Checkable
        aItem=new QStandardItem();
        aItem->setCheckable(true); //设置为Checkable
        aItem->setTextAlignment(Qt::AlignCenter);
        if (tmpList.at(j) == "0")
            aItem->setCheckState(Qt::Unchecked); //根据数据设置check状态
        else
            aItem->setCheckState(Qt::Checked);
        theModel->setItem(i-1,j,aItem); //为模型的某个行列位置设置Item
        //剩下列
        for (j = 1; j < SIGNTABLE_COLUMS; j++) //tmpList的行数等于FixedColumnCount, 固定的
        {
            //不包含最后一列
            aItem=new QStandardItem(tmpList.at(j));//创建item
            aItem->setTextAlignment(Qt::AlignCenter);
            theModel->setItem(i-1,j,aItem); //为模型的某个行列位置设置Item
        }
    }
}

void QFormSignTable::saveTable()
{
    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("signtable.txt"));
    if (!(aFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)))
        return; //以读写、覆盖原有内容方式打开文件

    QTextStream aStream(&aFile); //用文本流读取文件

    QStandardItem   *aItem;
    int i,j;
    QString str;

    //获取表头文字
    for (i = 0; i < theModel->columnCount(); i++)
    {
        aItem = theModel->horizontalHeaderItem(i); //获取表头的项数据
        str = str + aItem->text() + "\t\t";  //以TAB见隔开
    }
    aStream<<str<<"\n";  //文件里需要加入换行符 \n

    //获取数据区文字
    for (i = 0; i < theModel->rowCount(); i++)
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

QFormSignTable::~QFormSignTable()
{
    delete ui;
}

//添加
void QFormSignTable::on_pushButton_Add_clicked()
{
    //在表格最后添加行
    QList<QStandardItem*> aItemList; //容器类
    QStandardItem *aItem;

    //第一列
    aItem=new QStandardItem();
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setCheckable(true);
    aItemList<<aItem;   //添加到容器
    //后面列
    for(int i = 1; i < theModel->columnCount(); i++) //不包含最后1列
    {
        aItem=new QStandardItem("0"); //创建Item
        aItem->setTextAlignment(Qt::AlignCenter);
        aItemList<<aItem;   //添加到容器
    }

    theModel->insertRow(theModel->rowCount(), aItemList); //插入一行，需要每个Cell的Item
    QModelIndex curIndex = theModel->index(theModel->rowCount()-1, 0);//创建最后一行的ModelIndex
    theSelection->clearSelection();//清空选择项
    theSelection->setCurrentIndex(curIndex, QItemSelectionModel::Select);//设置刚插入的行为当前选择行
}

//删除
void QFormSignTable::on_pushButton_Del_clicked()
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

//保存并退出
void QFormSignTable::on_pushButton_SaveExit_clicked()
{
    saveTable();

//    QFormMain *ptr = (QFormMain*)parentWidget();//获取上一级父窗口 注意：反复调用 parentWidget 仍是 QFormDebug
//    //获取数据区文字 保存到table
//    for (int i = 0; i < theModel->rowCount(); i++)
//    {
//        int key = theModel->item(i, 1)->text().toInt();
//        ptr->SignTable[key] = theModel->item(i, 2)->text();
//    }
    this->close();
}

