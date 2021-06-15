#ifndef QFORMMAIN_H
#define QFORMMAIN_H

#include <QWidget>
#include <QLabel>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include "tcpserver.h"

namespace Ui {
class QFormMain;
}

class QFormMain : public QWidget
{
    Q_OBJECT

private:
    QLabel *LabCurFile;    //当前文件
    QLabel *LabCellPos;    //当前单元格行列号
    QLabel *LabCellText; //当前单元格内容
    QStandardItemModel * theModel; //数据模型
    QItemSelectionModel *theSelection; //选择模型

    void iniModelFromStringList (QStringList&) ; //从 StringList 初始化数据模型
    void saveTable();

    void loadSignTable();

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

    QMap<int, QString> SignTable;//表示语表

private slots:
    void on_pushButton_AddDevice_clicked();

    void on_pushButton_DelDevice_clicked();

    void on_pushButton_SaveTable_clicked();

    void on_pushButton_SignTable_clicked();

private:
    Ui::QFormMain *ui;
};

#endif // QFORMMAIN_H
