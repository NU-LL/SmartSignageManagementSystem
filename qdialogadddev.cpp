#include "qdialogadddev.h"
#include "ui_qdialogadddev.h"


#include "config.h"


#include <qdebug.h>
#include <QToolTip>
#include <QMessageBox>
#include <QInputDialog>


#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegExp>

#include "qformmain.h"


QStringList Group::GroupNameList;

//初始化列表
//需要一开始就调用，获得全局唯一的组表
void Group::Init()
{
    static bool initflag = false;
    if(initflag)//确保只初始化一次
        return ;

    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("groups.txt"));  //以文件方式读出
    //如果配置文件不存在则写入默认文件
    if(!aFile.exists()){
        aFile.open(QIODevice::WriteOnly);
        QTextStream aStream(&aFile); //用文本流读取文件
        aStream << tr("未命名分组");
        aFile.close();
    }
    if (aFile.open(QIODevice::ReadOnly | QIODevice::Text)) //以只读文本方式打开文件
    {
        QTextStream aStream(&aFile); //用文本流读取文件
        while (!aStream.atEnd())
        {
            QString str=aStream.readLine();//读取文件的一行
            addGroup(str);//添加到 StringList
        }
        aFile.close();//关闭文件
    }
    initflag = true;
}

//保存配置
bool Group::saveGroup()
{
    //清空并打开文件
    QFile aFile(QDir(DEFAULT_PROFILE_PATH).absoluteFilePath("groups.txt"));
    if (!(aFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)))
        return false;//以读写、覆盖原有内容方式打开文件

    //开始保存文件
    QTextStream aStream(&aFile); //用文本流读取文件
    //获取表头文字
    foreach(const QString& str, GroupNameList)
        aStream << str << "\n";//文件里需要加入换行符 \n
    return true;
}












QDialogAddDev::QDialogAddDev(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QDialogAddDev)
{
    ui->setupUi(this);

    //初始化表（该处属于多此一举 qformmain 中已经调用过一次）
    Group::Init();

//    this->installEventFilter(this);

    //tcp server
    TcpServer& server = TcpServer::getHandle();
    if(!server.serverStatus())
    {
        //服务未启动 关闭该窗口
        ui->comboBox_Device->setEnabled(false);
        ui->comboBox_Sign->setEnabled(false);
        ui->lineEdit_Name->setEnabled(false);
        ui->buttonBox->setEnabled(false);
        QMessageBox::warning(this, tr("警告"), tr("请先启动服务"));
        return;
    }
    //填充设备列表 comboBox_Device
    QList<QTcpSocket *> m_tcps = server.m_server.findChildren<QTcpSocket *>();
    foreach (QTcpSocket *tcp, m_tcps)
        ui->comboBox_Device->addItem(tcp->objectName());

    //找到当前设备的id并输出到label上
    QString ip = ui->comboBox_Device->currentText();
    ui->label_DeviceId->setText(server.findId(ip));

    //设置警示语列表
    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(ui->comboBox_Sign->model());
    int i = 0;
    foreach(Sign* sign, Sign::getSignTable())
    {
        ui->comboBox_Sign->addItem(sign->getIcon(), sign->text, QVariant::fromValue(sign));//添加图标 文字 自定义对象
        pItemModel->item(i++)->setForeground(sign->getColor());//修改某项文本颜色
    }
    //设置警示语列表切换时颜色也跟着改变
    Sign* sign = ui->comboBox_Sign->currentData().value<Sign*>();
    ui->comboBox_Sign->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));
    connect(ui->comboBox_Sign, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        //先获取信号的发送者
        QComboBox *cmb = qobject_cast<QComboBox *>(sender());
        Sign* sign = cmb->itemData(index).value<Sign*>();
        cmb->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));
    });

//    Sign::getSignTable()
//    QFormMain *ptr = (QFormMain*)parentWidget();//获取上一级父窗口 注意：反复调用 parentWidget 仍是 QFormDebug
//    for(const QString& str : qAsConst(ptr->SignTable))
////    foreach (auto str, ptr->SignTable)
//    {
//        ui->comboBox_Sign->addItem(str);
//    }

    //设置分组列表
    ui->comboBox_GroupName->addItems(Group::getGroupNameList());

}

QDialogAddDev::~QDialogAddDev()
{
    delete ui;
}


//bool QDialogAddDev::eventFilter(QObject *obj, QEvent *event)
//{
//    if (event->type() == QEvent::ToolTip) {
////		if (dynamic_cast<QLabel*>(obj) == ui->label) {
//            QToolTip::showText(QCursor::pos(), tr("测试测试"), this);
////		}
//    }
//    return QWidget::eventFilter(obj, event);
//}



const QString QDialogAddDev::getName()
{
    return ui->lineEdit_Name->text();
}

const QString QDialogAddDev::getDeviceIp()
{
    return ui->comboBox_Device->currentText();
}

const QString QDialogAddDev::getDeviceId()
{
    if(ui->label_DeviceId->text() == TcpServer::ERROR)//设备id错误
        return QString("");
    return ui->label_DeviceId->text();
}

const QString QDialogAddDev::getSignText()
{
    return ui->comboBox_Sign->currentText();
}

const QString QDialogAddDev::getSignIdx()
{
    Sign* sign = ui->comboBox_Sign->currentData().value<Sign*>();
    return sign->id;
}

const QString QDialogAddDev::getGroupName()
{
    return ui->comboBox_GroupName->currentText();
}

void QDialogAddDev::on_comboBox_Device_currentTextChanged(const QString &arg1)
{
    TcpServer& server = TcpServer::getHandle();
//    if(server.Ip2IdTable.find(arg1) != server.Ip2IdTable.end())
//        ui->label_DeviceId->setText(server.Ip2IdTable[arg1]);
    ui->label_DeviceId->setText(server.findId(arg1));
}


void QDialogAddDev::on_pushButton_AddGroup_clicked()
{
    bool ok=false;
    QString text = QInputDialog::getText(this, tr("添加组"),
                                         tr("请输入待添加的组名："), QLineEdit::Normal,
                                         QString(""), &ok);
    //只有当用户输入不为空且不在列表中存在时才插入
    if (ok && !text.isEmpty())
    {
        if(ui->comboBox_GroupName->findText(text) != -1)
        {
            QMessageBox::information(this, "提醒", "该分组已经存在，请重新添加");
            return ;
        }
        ui->comboBox_GroupName->addItem(text);
        Group::addGroup(text);
        Group::saveGroup();
    }
}



