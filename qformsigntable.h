#ifndef QFORMSIGNTABLE_H
#define QFORMSIGNTABLE_H

#include <QWidget>
#include <QStandardItemModel>
#include <QItemSelectionModel>

namespace Ui {
class QFormSignTable;
}

class QFormSignTable : public QWidget
{
    Q_OBJECT
private:
    QStandardItemModel * theModel; //数据模型
    QItemSelectionModel *theSelection; //选择模型

    void iniModelFromStringList (QStringList&) ; //从 StringList 初始化数据模型
    void saveTable();

public:
    explicit QFormSignTable(QWidget *parent = nullptr);
    ~QFormSignTable();

private slots:
    void on_pushButton_Add_clicked();

    void on_pushButton_Del_clicked();

    void on_pushButton_SaveExit_clicked();

private:
    Ui::QFormSignTable *ui;
};

#endif // QFORMSIGNTABLE_H
