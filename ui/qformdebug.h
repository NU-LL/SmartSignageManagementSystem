#ifndef QFORMDEBUG_H
#define QFORMDEBUG_H

#include <QWidget>
#include <QTcpSocket>
#include <QMessageBox>

namespace Ui {
class QFormDebug;
}

class QFormDebug : public QWidget
{
    Q_OBJECT

public:
    //单例模式
    //注意：该函数线程不安全（C++构造函数本身就是线程不安全）
//    static QFormDebug* instance()
//    {
//        static QFormDebug *formDebug = new QFormDebug;
//        return formDebug;
//    }
    explicit QFormDebug(QWidget *parent = nullptr);
    ~QFormDebug();

    static void showMessageBox(QMessageBox::Icon icon, QString title, QString text, int ms = 0, bool isBlock = false);


    QString hexDump(const QByteArray &data, bool ishtml = false);

    QString getObjectName();//获得现在的对象名称
private:




public slots:
    void recMessage(int type, void* message = nullptr);//消息接收槽函数
    void recData(QTcpSocket* tcp, const QByteArray& data);//数据接收槽函数

private slots:
    void on_pushButton_StartServer_clicked();

    void on_pushButton_Send_clicked();

    void on_pushButton_CustomSend_clicked();

    void on_pushButton_DebugCmd_clicked();

    void on_comboBox_DeviceInfo_currentTextChanged(const QString &arg1);

private:
    Ui::QFormDebug *ui;
};

#endif // QFORMDEBUG_H
