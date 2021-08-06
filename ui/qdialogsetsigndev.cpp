#include "qdialogsetsigndev.h"
#include "ui_qdialogsetsigndev.h"

#include <QDir>
#include <QFile>
#include <QColor>
#include <QStandardItemModel>

#include <QInputDialog>
#include <QMessageBox>

#include <QDebug>


#include "qformsigntable.h"
#include "qdialogadddev.h"


QDialogSetSignDev::QDialogSetSignDev(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QDialogSetSignDev)
{
    ui->setupUi(this);


    //初始化id


    //初始化分组下拉框
    ui->comboBox_GroupName->addItems(Group::getGroupNameList());


    //初始化名称


    //初始化标示语下拉框
    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(ui->comboBox_Sign->model());
    int i = 0;
    foreach(Sign* sign, Sign::getSignTable())
    {
        ui->comboBox_Sign->addItem(sign->getIcon(), sign->text, QVariant::fromValue(sign));//添加图标 文字 自定义对象
        pItemModel->item(i++)->setForeground(sign->getColor());//修改某项文本颜色
    }
    //设置警示语列表切换时颜色也跟着改变
    Sign* sign = ui->comboBox_Sign->currentData().value<Sign*>();
    if(sign != nullptr)
        ui->comboBox_Sign->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));
    connect(ui->comboBox_Sign, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        //先获取信号的发送者
        QComboBox *cmb = qobject_cast<QComboBox *>(sender());
        Sign* sign = cmb->itemData(index).value<Sign*>();
        if(sign != nullptr)
            cmb->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));
    });

    //初始化三个checkbox

}

QDialogSetSignDev::~QDialogSetSignDev()
{
    delete ui;
}




void QDialogSetSignDev::setParameters(TcpSignDevice *dev)
{
    Sign *sign = Sign::findSign(dev->signid);
    if(sign == nullptr)
    {
        QMessageBox::critical(this, "错误", "找不到对应的标示语id："+dev->signid);
        return ;
    }
    this->dev = dev;

    ui->label_Id->setText(dev->id);
    ui->comboBox_GroupName->setCurrentText(dev->groupname);//如果无效，默认设置为第一个
    ui->lineEdit_Name->setText(dev->name);
    ui->comboBox_Sign->setCurrentText(sign->text);//如果无效，默认设置为第一个

    ui->checkBox_Voice->setChecked(dev->voice==1);
    ui->checkBox_Flash->setChecked(dev->flash==1);
    ui->checkBox_Alert->setChecked(dev->alert==1);
}

//保存对象
void QDialogSetSignDev::on_buttonBox_accepted()
{
    dev->groupname = ui->comboBox_GroupName->currentText();
    dev->name = ui->lineEdit_Name->text();
    dev->signid = ui->comboBox_Sign->currentData().value<Sign*>()->id;
    dev->voice = ui->checkBox_Voice->isChecked() == true?1:0;
    dev->flash = ui->checkBox_Flash->isChecked() == true?1:0;
    dev->alert = ui->checkBox_Alert->isChecked() == true?1:0;
}

