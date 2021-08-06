#include "qdialogsetsign.h"
#include "ui_qdialogsetsign.h"



#include <QDir>
#include <QFile>
#include <QColor>
#include <QStandardItemModel>

#include <QInputDialog>
#include <QMessageBox>

#include <QDebug>

#include "qformoptions.h"
#include "qformsigntable.h"



QDialogSetSign::QDialogSetSign(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QDialogSetSign)
{
    ui->setupUi(this);


    //id初始化


    //标示语初始化


    //颜色初始化
    ui->comboBox_Color->addItems(Sign::colorstrzh);
    {
        //设置下拉框颜色
        QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(ui->comboBox_Color->model());
        Sign sign;
        for (int i = 0; i < ui->comboBox_Color->count(); i++)
        {
            sign.color = i;
            pItemModel->item(i)->setForeground(sign.getColor());//修改某项文本颜色
    //        pItemModel->item(i)->setBackground(sign.getColor());//修改某项背景颜色
        }
    }
    {
        //设置当前的背景色
        int idx = ui->comboBox_Color->currentIndex();
        ui->comboBox_Color->setStyleSheet(QString("QComboBox{background:%1}").arg(Sign::colorstren[idx]));
        connect(ui->comboBox_Color, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx){
            //先获取信号的发送者
            QComboBox *cmb = qobject_cast<QComboBox *>(sender());
            cmb->setStyleSheet(QString("QComboBox{background:%1}").arg(Sign::colorstren[idx]));
        });
    }

    //图标初始化
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
    ui->comboBox_Icon->addItem("无图标");
    foreach(const QString& iconfilename, files)
        ui->comboBox_Icon->addItem(QIcon(dir.absoluteFilePath(iconfilename)), iconfilename);
}

QDialogSetSign::~QDialogSetSign()
{
    delete ui;
}







void QDialogSetSign::setParameters(Sign * sign)
{
    this->sign = sign;
    ui->lineEdit_Id->setText(sign->id);
    ui->lineEdit_SignText->setText(sign->text);
    ui->comboBox_Color->setCurrentIndex(sign->color);
    ui->comboBox_Icon->setCurrentText(sign->iconfilename);//如果无效，默认设置为第一个（图标的第一个为 “无图标”）
}

void QDialogSetSign::setIdEnabled(bool idEdit)
{
    ui->lineEdit_Id->setEnabled(idEdit);
}

//保存对象
void QDialogSetSign::on_buttonBox_accepted()
{
    //保存信息到对象中
    sign->id = ui->lineEdit_Id->text();//仅仅在一开始添加时 lineEdit_Id 才会允许编辑 该项才有机会修改
    sign->text = ui->lineEdit_SignText->text();
    if(ui->comboBox_Icon->currentText() == "无图标")
        sign->iconfilename = QString();
    else
        sign->iconfilename = ui->comboBox_Icon->currentText();
    sign->color = ui->comboBox_Color->currentIndex();
}

