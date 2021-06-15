#include "qdialogadddev.h"
#include "ui_qdialogadddev.h"

#include <qdebug.h>
#include <QMessageBox>

#include "qformmain.h"

QDialogAddDev::QDialogAddDev(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QDialogAddDev)
{
    ui->setupUi(this);

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
    //填充 comboBox_Device
    QList<QTcpSocket *> m_tcps = server.m_server.findChildren<QTcpSocket *>();
    foreach (QTcpSocket *tcp, m_tcps)
    {
        ui->comboBox_Device->addItem(tcp->objectName());
    }

    QFormMain *ptr = (QFormMain*)parentWidget();//获取上一级父窗口 注意：反复调用 parentWidget 仍是 QFormDebug
//    for(auto str : ptr->SignTable)
    foreach (auto str, ptr->SignTable)
    {
        ui->comboBox_Sign->addItem(str);
    }

}

QDialogAddDev::~QDialogAddDev()
{
    delete ui;
}

QString QDialogAddDev::getName()
{
    return ui->lineEdit_Name->text();
}

QString QDialogAddDev::getDevice()
{
    return ui->comboBox_Device->currentText();
}

QString QDialogAddDev::getSign()
{
    return ui->comboBox_Sign->currentText();
}
