#include "qformsigntable.h"
#include "ui_qformsigntable.h"

#include "config.h"

#include <QStandardItemModel>
#include <QItemSelectionModel>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QColor>

#include <QInputDialog>
#include <QMessageBox>


#include "qformmain.h"
#include "qdialogsetsign.h"


QMap<QString, Sign*> Sign::SignTable;
//Sign对象中的color成员 对应的颜色表如下：
QStringList Sign::colorstrzh = {"黑色", "红色", "绿色", "黄色", "蓝色", "洋红色", "青色", "白色"};
QStringList Sign::colorstren = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};

//初始化列表
//需要一开始就调用，获得全局唯一的警示语表
void Sign::Init()
{
    static bool initflag = false;
    if(initflag)//确保只初始化一次
        return ;

//    QStringList fFileContent;//文件内容字符串列表
//    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("signtable.txt"));  //以文件方式读出
//    //如果配置文件不存在则写入默认文件
//    if(!aFile.exists()){
//        aFile.open(QIODevice::WriteOnly);
//        QTextStream aStream(&aFile); //用文本流读取文件
//        aStream << tr("id\t\t标示语");
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
//            Sign* sign = new Sign();
//            sign->deserialization(fFileContent.at(i));//获取数据区的一行 并进行反序列化
//            SignTable[sign->id] = sign;//添加到表中
//        }
//    }


    //json文件存储
    QFile loadFile(QDir(QFormOptions::ConfigFilePath).absoluteFilePath("signtable.json"));
    if(!loadFile.exists())
    {
        loadFile.open(QIODevice::WriteOnly);
        loadFile.close();
    }
    if (!loadFile.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open load file : signtable.json.");
        return;
    }
    QByteArray loadData = loadFile.readAll();
    loadFile.close();
    QJsonDocument loadDoc(QJsonDocument::fromJson(loadData));
//    QJsonDocument loadDoc(QJsonDocument::fromBinaryData(saveData));

    QJsonObject json = loadDoc.object();
    QJsonArray signDevArray = json["SignTable"].toArray();
    for (int idx = 0; idx < signDevArray.size(); ++idx)
    {
        QJsonObject signDevObj = signDevArray[idx].toObject();
        Sign* sign = new Sign();
        sign->deserialization(signDevObj);//获取数据区的一行 并进行反序列化
        SignTable[sign->id] = sign;//添加到表中
    }

    initflag = true;
}

//保存警示表
bool Sign::save()
{
    //json文件存储（注意，会清空）
    QFile saveFile(QDir(QFormOptions::ConfigFilePath).absoluteFilePath("signtable.json"));
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        qWarning("Couldn't open save file : signtable.json.");
        return false;
    }
    //遍历整个警示语表 存入json
    QJsonObject tableObj;
    QJsonArray signDevArray;
    foreach(Sign* sign, SignTable)
    {
        //序列化警示语对象 保存到文本中
        QJsonObject signDevObj;
        sign->serialization(signDevObj);
        signDevArray.append(signDevObj);
    }
    tableObj["SignTable"] = signDevArray;
    QJsonDocument saveDoc(tableObj);
    saveFile.write(saveDoc.toJson());
//    saveFile.write(saveDoc.toBinaryData());
    saveFile.close();


//    //记录表头 确保不会被清空
//    QString tablehead;
//    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("signtable.txt"));  //以文件方式读出
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
//    foreach(Sign* sign, SignTable)
//        //序列化警示语对象 保存到文本中
//        aStream << sign->serialization();
    return true;
}

//序列化
const QString Sign::serialization() const
{
    QString str;

    str += this->id;
    str += "\t\t";
    str += this->text;
    str += "\t\t";
    str += QString("%1").arg(this->color);
    str += "\t\t";
    if(this->iconfilename == QString(""))
        str += QString("/");
    else
        str += this->iconfilename;
    str += "\n";

    return str;
}

void Sign::serialization(QJsonObject &json) const
{
    json["id"] = this->id;
    json["text"] = this->text;
    json["color"] = this->color;
    json["iconfilename"] = this->iconfilename;
}

//反序列化
void Sign::deserialization(const QString &str)
{
    //一个或多个空格、TAB等分隔符隔开的字符串， 分解为一个StringList
    const QStringList& tmpList = str.split(QRegExp("\\s+"),Qt::SkipEmptyParts);
    //赋值
    this->id = tmpList[0];
    this->text = tmpList[1];
    this->color = tmpList[2].toUShort();
    if(tmpList[3] == QString("/"))
        this->iconfilename = QString("");
    else
        this->iconfilename = tmpList[3];
}

void Sign::deserialization(const QJsonObject &json)
{
    this->id = json["id"].toString();
    this->text = json["text"].toString();
    this->color = json["color"].toInt();
    this->iconfilename = json["iconfilename"].toString();
}








//创建控件
QWidget *signDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QComboBox *editor = new QComboBox(parent);

//    QWidget *editor = new QWidget(parent);

//    editor->setStyleSheet("QComboBox{color:red}");

    editor->addItems({"1", "2", "3", "4"});
//    for(auto it = SignTable.begin(); it != SignTable.end(); ++it)
//        editor->addItem(SignTable[it.key()]);
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
    model->setData(index, text);
//    model->setData(index, QIcon("H:\\CODE\\QT\\build-SmartSignageManagementSystem-Desktop_Qt_5_15_2_MinGW_32_bit-Debug\\Icon\\guanbi2fill.png"));
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
//    //模态显示对话框
//    QMessageBox* msgBox = new QMessageBox( mainWindow );
//    msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
//    msgBox->setStandardButtons( QMessageBox::Ok );
//    msgBox->setWindowTitle(tr("提示"));
//    msgBox->setText(tr("是否更换标示语"));
//    msgBox->setModal( false ); // if you want it non-modal

//    msgBox->setIcon(QMessageBox::Question);
//    msgBox->show();
}


















QFormSignTable::QFormSignTable(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormSignTable)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除


    theModel = new QStandardItemModel(0, 0, this); //数据模型
    theSelection = new QItemSelectionModel(theModel);//选择模型

    ui->tableView->setModel(theModel); //设置数据模型
    ui->tableView->setSelectionModel(theSelection); //设置选择模型

    //设置代理
    signDelegate signdelegate;
//    ui->tableView->setItemDelegateForColumn(0, &signdelegate);

    //初始化表（该处属于多此一举 qformmain 中已经调用过一次）
    Sign::Init();

    //加载表格
    theModel->setHorizontalHeaderLabels({"id", "标示语"}); //设置表头文字
    foreach(Sign* sign, Sign::getSignTable())
        addLine(sign->id, sign->text, sign->getColor(), sign->getIcon());//添加一行到表格中
}

//设置颜色下拉框
void QFormSignTable::setColorCombox(QComboBox *combobox)
{
    combobox->addItems({"0x00","0x01","0x02","0x03","0x04","0x05","0x06","0x07"});

    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(combobox->model());
    Sign sign;
    for (int i = 0; i < combobox->count(); i++)
    {
        sign.color = i;
        pItemModel->item(i)->setForeground(sign.getColor());//修改某项文本颜色
//        pItemModel->item(i)->setBackground(sign.getColor());//修改某项背景颜色
    }
//    for (int i = 0; i < combobox->count(); i++) {
////            pItemModel->item(i)->setIcon(QIcon(":/TestComboBox/Resources/deletered.png"));    //修改某项图标
////            pItemModel->item(i)->setText("修改的文本  " + QString::number(i + 1));                          //修改某项文本
//        QString str = pItemModel->item(i)->text();
//        pItemModel->item(i)->setForeground(QColor(255, 0, 0));            //修改某项文本颜色
//        pItemModel->item(i)->setBackground(QColor(220,220,220));    //修改某项背景颜色
////            pItemModel->item(i)->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);    //修改某项文本对齐方式
//    }
//    combobox->setStyleSheet("QComboBox{background:black}");
    sign.color = combobox->currentData().toString().toUShort(nullptr, 16);
    combobox->setStyleSheet(QString("QComboBox{background:%1}").arg(sign.getStrColor()));
    connect(combobox, &QComboBox::currentTextChanged, this, [this](const QString& str){
        Sign sign;
        sign.color = str.toUShort(nullptr, 16);
        //先获取信号的发送者
        QComboBox *cmb = qobject_cast<QComboBox *>(sender());
        cmb->setStyleSheet(QString("QComboBox{background:%1}").arg(sign.getStrColor()));

        //设置字体颜色
//        QPalette plt = widget->palette();
//        plt.setColor(QPalette::Text, sign.getColor());
//        widget->setPalette(plt);
    });
}

//设置图标下拉框
void QFormSignTable::setIconCombox(QComboBox *combobox)
{
    static QDir dir = QDir(QFormOptions::IconPath);
    static QStringList files = dir.entryList(QStringList("*.png"), QDir::Files|QDir::Readable, QDir::Name);
    //如果路径更新 则更新files
    //注意：软件运行期间不检查文件更新，如果文件夹下文件更新（增加或减少），请重启该软件
    if(QFormOptions::IconPath != dir.absolutePath())
    {
        dir = QDir(QFormOptions::IconPath);
        files = dir.entryList(QStringList("*.png"), QDir::Files|QDir::Readable, QDir::Name);
        qDebug() << "更新图标列表";
    }

    combobox->addItem("无图标");
    combobox->addItems(files);

    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(combobox->model());
    Sign sign;
    for (int i = 1; i < combobox->count(); i++)
        pItemModel->item(i)->setIcon(QIcon(dir.absoluteFilePath(files[i-1])));
//    combobox->setIconSize(QSize(32, 32));
}

QFormSignTable::~QFormSignTable()
{
    delete ui;
}
















//添加一行
void QFormSignTable::addLine(const QString& id, const QString& text, const QColor& color, const QIcon& icon)
{
    //在表格最后添加行
    QList<QStandardItem*> aItemList; //容器类
    QStandardItem *aItem;

    //id
    aItem=new QStandardItem(id);//创建item
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止修改
    aItemList << aItem;
    //警示语
//    aItem=new QStandardItem(text);//创建item
//    aItem->setTextAlignment(Qt::AlignCenter);
//    aItem->setEditable(false);//禁止修改
////        aItem->setForeground(Qt::red);//设置字体颜色
////        aItem->setIcon(QIcon(signline.getPixmap()));//设置图标
//    aItemList << aItem;

    aItem=new QStandardItem(text);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItem->setForeground(color);//设置字体颜色
    aItem->setIcon(icon);//设置图标
    aItemList << aItem;

    //添加到新的一行
    theModel->insertRow(theModel->rowCount(), aItemList); //插入一行，需要每个Cell的Item
    QModelIndex curIndex = theModel->index(theModel->rowCount()-1, 0);//创建最后一行的ModelIndex
    theSelection->clearSelection();//清空选择项
    theSelection->setCurrentIndex(curIndex, QItemSelectionModel::Select);//设置刚插入的行为当前选择行

//    //颜色
////        aItem=new QStandardItem(QString("0x%1").arg(signline.color, 2, 16, QLatin1Char('0')));//创建item
////        aItem->setTextAlignment(Qt::AlignCenter);
////        aItem->setBackground(signline.getColor());
//////        aItem->setBackground(Qt::yellow);//设置背景色
////        theModel->setItem(i-1, 2, aItem); //为模型的某个行列位置设置Item
//    QComboBox* combobox=new QComboBox(this);
//    setColorCombox(combobox);
//    combobox->setCurrentText(QString("0x%1").arg(color, 2, 16, QLatin1Char('0')));//设置对应的颜色
//    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 2), combobox);
//    //图标名字
////        aItem=new QStandardItem(signline.iconname);//创建item
////        aItem->setTextAlignment(Qt::AlignCenter);
////        theModel->setItem(i-1, 3, aItem); //为模型的某个行列位置设置Item
//    combobox=new QComboBox(this);
//    setIconCombox(combobox);
//    combobox->setCurrentText(iconfilename);//如果没有 默认设置为第一项
//    ui->tableView->setIndexWidget(theModel->index(theModel->rowCount()-1, 3), combobox);
}

//添加
void QFormSignTable::on_pushButton_Add_clicked()
{
    //todo 同步问题待解决
    //如何做到用户在表格中修改后实时同步到Sign中？
    //formmain中也存在这个问题

    Sign* sign = new Sign(QString("%1").arg(theModel->rowCount()+1, 3, 10, QLatin1Char('0')), QString(""), QString(""), 0);

    //弹窗
    QDialogSetSign *dlgSign = new QDialogSetSign(this);
    dlgSign->setParameters(sign);
    int ret = dlgSign->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)//OK按钮被按下
    {
        if(sign->id.isEmpty())
        {
            QMessageBox::warning(this, "警告", "添加失败（id为空）");
            goto error;
        }

        if(Sign::findSign(sign->id)!= nullptr)
        {
            QMessageBox::warning(this, "警告", "添加失败（id不唯一）");
            goto error;
        }
        addSign(sign);
    }
    delete dlgSign;
    return ;
error:
    delete dlgSign;
    delete sign;
}

//删除
void QFormSignTable::on_pushButton_Del_clicked()
{
    //删除行
    QModelIndex curIndex=theSelection->currentIndex();//获取模型索引
    if(!curIndex.isValid())
    {
        QMessageBox::information(this, "通知", "列表为空，不能再删除");
        return ;
    }
    int counts = theModel->rowCount();
    delSign(curIndex.row());

    //如果不是删除最后一行 则重新设置当前选择行
    if(curIndex.row () != counts - 1)
        theSelection->setCurrentIndex (curIndex, QItemSelectionModel::Select);
}

//保存并退出
void QFormSignTable::on_pushButton_SaveExit_clicked()
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
        QMessageBox::warning(this, tr("警告"), tr("保存失败（请确保id唯一）"));
        return;
    }

    //保存数据
    Sign::save();

    QMessageBox::information(this, tr("通知"), tr("保存成功"));

    this->close();
}



//双击修改对象
void QFormSignTable::on_tableView_doubleClicked(const QModelIndex &index)
{
    //获得该行对应的对象
    QString id = theModel->data(theModel->index(index.row(), 0)).toString();//获得该行的id
    Sign* sign = Sign::findSign(id);
    //弹窗
    QDialogSetSign *dlgSign = new QDialogSetSign(this);
    dlgSign->setParameters(sign);
    dlgSign->setIdEnabled(false);//修改时id禁止修改
    int ret = dlgSign->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)//OK按钮被按下
    {
        //刷新表格数据
        QStandardItem* signItem = theModel->item(index.row(), 1);//警示语栏
        signItem->setForeground(sign->getColor());//设置字体颜色
        signItem->setIcon(sign->getIcon());//设置图标
        signItem->setText(sign->text);
    }
    delete dlgSign;
}

