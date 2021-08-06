#ifndef QFORMMAIN_H
#define QFORMMAIN_H

#include <QWidget>
#include <QLabel>
#include <QBitArray>
#include <QCloseEvent>
#include "device/tcpsigndevice.h"

#include <QMessageBox>

#include "qformsigntable.h"

namespace Ui {
class QFormMain;
}

class QFormMain : public QWidget
{
    Q_OBJECT

private:
    QStandardItemModel * theModel; //数据模型
    QItemSelectionModel *theSelection; //选择模型

    //事件截获
    bool eventFilter(QObject *watched, QEvent *event);
    //关闭窗口信号
    void closeEvent(QCloseEvent *event);
    //修改某一项
    bool modifyCell(int row, int column, const QString &text, const QIcon &icon = QIcon(), const QBrush &brush = QBrush(Qt::white)){
        QStandardItem* aItem = theModel->item(row, column);
        if(aItem == nullptr)
        {
            QMessageBox::warning(this, "警告", QString("修改(%1, %2)处失败，超出范围").arg(row).arg(column));
            return false;
        }
        bool res = false;
        if(aItem->text() != text)
        {
            aItem->setText(text);
            res = true;
        }
        if(!icon.isNull())
        {
            aItem->setIcon(icon);
            res = true;
        }
//        if(aItem->foreground() != brush)
        if(theModel->data(aItem->index(), Qt::BackgroundColorRole).value<QBrush>() != brush)
        {
//            aItem->setForeground(brush);//设置文字颜色
            theModel->setData(aItem->index(), brush, Qt::BackgroundColorRole);//设置颜色
            res = true;
        }
        return res;
    };

public:
//    //单例模式
//    //注意：该函数线程不安全（C++构造函数本身就是线程不安全）
//    static QFormMain* instance()
//    {
//        static QFormMain *formSignTable = new QFormMain;
//        return formSignTable;
//    }
    explicit QFormMain(QWidget *parent = nullptr);
    ~QFormMain();

    //更新 SignDevice 中的数据到表格中
    //返回值：哪一位置1则表示该列更新
    QBitArray refreshSignDev(TcpSignDevice *dev);


    void addDevice(TcpSignDevice* dev)
    {
        dev->item = addLine(dev);
    };
    void delDevice(int row)
    {
        QString id = theModel->item(row,0)->text();
        delLine(row);
    };


private:
    //设置分组下拉框
    void setGroupCombox(QComboBox *combobox);
    //设置警示语下拉框
    void setSignCombox(QComboBox *combobox);
    //设置开关选项下拉框
    void setSwitchCombox(QComboBox *combobox);

    //添加一行
    //返回第一栏（id）的指针（插入失败返回空）
    QStandardItem* addLine(const TcpSignDevice *dev);
    //删除一行
    void delLine(int row){theModel->removeRow(row);};



public slots:
    void recMessage(int type, void* message = nullptr);//消息接收槽函数

private slots:
    void on_pushButton_AddDevice_clicked();

    void on_pushButton_DelDevice_clicked();

    void on_pushButton_SaveTable_clicked();

    void on_pushButton_SignTable_clicked();

    void on_tableView_clicked(const QModelIndex &index);

    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    Ui::QFormMain *ui;
};

#endif // QFORMMAIN_H
