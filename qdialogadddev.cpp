#include "qdialogadddev.h"
#include "ui_qdialogadddev.h"

#include <qdebug.h>
#include <QToolTip>
#include <QMessageBox>

#include "qformmain.h"

QDialogAddDev::QDialogAddDev(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QDialogAddDev)
{
    ui->setupUi(this);

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
    //填充 comboBox_Device
    QList<QTcpSocket *> m_tcps = server.m_server.findChildren<QTcpSocket *>();
    foreach (QTcpSocket *tcp, m_tcps)
        ui->comboBox_Device->addItem(tcp->objectName());

    QString ip = ui->comboBox_Device->currentText();
    if(server.Ip2IdTable.find(ip) != server.Ip2IdTable.end())
        ui->label_DeviceId->setText(server.Ip2IdTable[ip]);

    QFormMain *ptr = (QFormMain*)parentWidget();//获取上一级父窗口 注意：反复调用 parentWidget 仍是 QFormDebug
    for(const QString& str : qAsConst(ptr->SignTable))
//    foreach (auto str, ptr->SignTable)
    {
        ui->comboBox_Sign->addItem(str);
    }

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



QString QDialogAddDev::getName()
{
    return ui->lineEdit_Name->text();
}

QString QDialogAddDev::getDeviceIp()
{
    return ui->comboBox_Device->currentText();
}

QString QDialogAddDev::getDeviceId()
{
    return ui->label_DeviceId->text();
}

QString QDialogAddDev::getSign()
{
    return ui->comboBox_Sign->currentText();
}

void QDialogAddDev::on_comboBox_Device_currentTextChanged(const QString &arg1)
{
    TcpServer& server = TcpServer::getHandle();
    if(server.Ip2IdTable.find(arg1) != server.Ip2IdTable.end())
        ui->label_DeviceId->setText(server.Ip2IdTable[arg1]);
}

