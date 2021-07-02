#ifndef QFORMMAIN_H
#define QFORMMAIN_H

#include <QWidget>
#include <QLabel>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QCloseEvent>
#include <QItemDelegate>
#include "tcpserver.h"

namespace Ui {
class QFormMain;
}


//标示语代理
class signDelegate: public QItemDelegate
{
    Q_OBJECT
public:
    signDelegate(QMap<int, QString>& SignTable, QObject *parent = 0):SignTable(SignTable){};

public:
    //创建控件
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    //设置控件显示的数据 将Model中的数据更新到Delegate中
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    //将Delegate中的数据更新到Model中
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    //更新控件区的显示
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const{
        editor->setGeometry(option.rect);
    };
private:
    QMap<int, QString>& SignTable;

private slots:
    void currentTextChanged(const QString &str);
    void currentIndexChanged(int idx);
};
//按键代理
class buttonDelegate: public QItemDelegate
{
    Q_OBJECT
public:
    buttonDelegate(QObject *parent = 0){};

public:
    //创建控件
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    //设置控件显示的数据 将Model中的数据更新到Delegate中
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    //将Delegate中的数据更新到Model中
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    //更新控件区的显示
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const{
        editor->setGeometry(option.rect);
    };
};







class QFormMain : public QWidget
{
    Q_OBJECT

private:
    QStandardItemModel * theModel; //数据模型
    QItemSelectionModel *theSelection; //选择模型
    QWidget *checkbox = nullptr;//第一栏的checkbox
    QWidget *button = nullptr;//最后一栏的按键

    void iniModelFromStringList (QStringList&) ; //从 StringList 初始化数据模型
    void saveTable();

    void loadSignTable();

    //关闭窗口信号
    void closeEvent(QCloseEvent *event);
    //修改某一项
    void modifyCell(int row, int column, const QString &text){
        QStandardItem* aItem = theModel->item(row, column);
        if(aItem)
            aItem->setText(text);
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

    //设置状态
    void setStatus(int row, quint8 staBytes);
    //添加设备
    void addDevice(const QString& id, const QString& name, const QString& Sign, quint8 staBytes);
    void addDevice(const QString& id, const QString& name, const QString& Sign, bool offline, bool voice, bool flash, bool alert){
        quint8 staBytes = (offline == true?0x08:0x00)|(voice == true?0x04:0x00)|(flash == true?0x02:0x00)|(alert == true?0x01:0x00);
        addDevice(id, name, Sign, staBytes);
    };

    QMap<int, QString> SignTable;//表示语表

public slots:
    void recMessage(int level, QString title, QString text, int message_id = MESSAGE_BOX, void* message = nullptr);//消息接收槽函数

private slots:
    void on_pushButton_AddDevice_clicked();

    void on_pushButton_DelDevice_clicked();

    void on_pushButton_SaveTable_clicked();

    void on_pushButton_SignTable_clicked();

    void on_tableView_clicked(const QModelIndex &index);

private:
    Ui::QFormMain *ui;
};

#endif // QFORMMAIN_H
