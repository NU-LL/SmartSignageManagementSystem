#ifndef QFORMSIGNTABLE_H
#define QFORMSIGNTABLE_H

#include <QWidget>
#include <QComboBox>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QCloseEvent>
#include <QItemDelegate>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QtDebug>

#include "qformoptions.h"

namespace Ui {
class QFormSignTable;
}



class Sign: public QObject
{
    Q_OBJECT
public:
    Sign(const QString& id, const QString& text, const QString& iconfilename, quint8 color = 0, QObject *parent = nullptr)
        :QObject(parent), id(id), text(text), iconfilename(iconfilename), color(color){};
    Sign(QObject *parent = nullptr):QObject(parent){};
    Sign(Sign&){};//拷贝构造 允许拷贝
    ~Sign(){};

public:
    QString id;
    QString text;//警示语
//    int iconidx;//图标在files中的偏移
    QString iconfilename;//图标文件名字

    //颜色
    union{
        struct{
            quint8 red:1;//最低位
            quint8 green:1;
            quint8 blue:1;//最高位
            quint8 undefined:5;
        };
        quint8 color;//颜色
    };

    //颜色表
    static QStringList colorstrzh;
    static QStringList colorstren;


public:
    //序列化
    const QString serialization() const;
    void serialization(QJsonObject& json) const;
    //反序列化
    void deserialization(const QString& str);
    void deserialization(const QJsonObject& json);


    //获得图标句绝对路径
    const QString getIconAbsolutePath() const { return QDir(QFormOptions::IconPath).absoluteFilePath(iconfilename);};
    //获得图标名字
    const QString geticonbasename() const { return QFileInfo(iconfilename).baseName(); };
    //获得icon对应的数据
    const QPixmap getPixmap() const {return QPixmap(this->getIconAbsolutePath());};
    //获得图标
    const QIcon getIcon() const {return QIcon(this->getIconAbsolutePath());};



    //获得对应的颜色
    Qt::GlobalColor getQtColor()
    {
        if(red == 1 && green == 0 && blue == 0)
            return Qt::red;
        if(red == 0 && green == 1 && blue == 0)
            return Qt::green;
        if(red == 0 && green == 0 && blue == 1)
            return Qt::blue;
        if(red == 1 && green == 1 && blue == 0)
            return Qt::yellow;
        if(red == 0 && green == 1 && blue == 1)
            return Qt::cyan;
        if(red == 1 && green == 0 && blue == 1)
            return Qt::magenta;
        if(red == 1 && green == 1 && blue == 1)
            return Qt::white;
        if(red == 0 && green == 0 && blue == 0)
            return Qt::black;
        return Qt::white;
    };
    QColor getColor()
    {
        return QColor(red == 1?0xff:0x00, green == 1?0xff:0x00, blue == 1?0xff:0x00);
    }
    const QString getStrColor()
    {
        return colorstren[color];
//        if(red == 1 && green == 0 && blue == 0)
//            return "red";
//        if(red == 0 && green == 1 && blue == 0)
//            return "green";
//        if(red == 0 && green == 0 && blue == 1)
//            return "blue";
//        if(red == 1 && green == 1 && blue == 0)
//            return "yellow";
//        if(red == 0 && green == 1 && blue == 1)
//            return "cyan";
//        if(red == 1 && green == 0 && blue == 1)
//            return "magenta";
//        if(red == 1 && green == 1 && blue == 1)
//            return "white";
//        if(red == 0 && green == 0 && blue == 0)
//            return "black";
//        return "white";
    };

public:
    //初始化列表
    //需要一开始就调用，获得全局唯一的警示语表
    static void Init();
    //保存警示表
    static bool save();

    //通过id找标示语对象
    static Sign* findSign(const QString& id)
    {
        auto iter = SignTable.find(id);
        if(iter != SignTable.end())
            return iter.value();
        return nullptr;
    };
    //注册警示语到警示语表中
    static void registerSign(const QString& id, Sign* sign){SignTable[id] = sign;};
    static void unregisterSignDev(const QString& id){delete SignTable[id];SignTable.remove(id);};
    //清空整个表
    static void clearSignTable()
    {
        for(Sign* it : SignTable)
            delete it;
        SignTable.clear();
    };
    //获取整个表，仅仅只有可读权限
    //注意，由于表中存的的对象指针，仍可以通过该指针修改设备（这里仅仅是不能修改这个map）
    static const QMap<QString, Sign*>& getSignTable(){return SignTable;};

private:

    //警示语表
    static QMap<QString, Sign*> SignTable;
};




////标示语代理
//class comboxDelegate: public QItemDelegate
//{
//    Q_OBJECT
//public:
//    signDelegate(Sign sign, QMap<int, QString>& SignTable, QObject *parent = 0):sign(sign), SignTable(SignTable){};

//public:
//    //创建控件
//    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
//    //设置控件显示的数据 将Model中的数据更新到Delegate中
//    void setEditorData(QWidget *editor, const QModelIndex &index) const;
//    //将Delegate中的数据更新到Model中
//    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
//    //更新控件区的显示
//    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const{
//        editor->setGeometry(option.rect);
//    };
//private:
//    Sign sign;
//    QMap<int, QString>& SignTable;


//private slots:
//    void currentTextChanged(const QString &str);
//    void currentIndexChanged(int idx);
//};
//标示语代理
class signDelegate: public QItemDelegate
{
    Q_OBJECT
public:
//    signDelegate(Sign sign, QMap<int, QString>& SignTable, QObject *parent = 0):sign(sign), SignTable(SignTable){};
    signDelegate(QObject *parent = 0){};

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
//    Sign sign;
//    QMap<int, QString>& SignTable;


private slots:
    void currentTextChanged(const QString &str);
    void currentIndexChanged(int idx);
};










class QFormSignTable : public QWidget
{
    Q_OBJECT
private:
    QStandardItemModel * theModel; //数据模型
    QItemSelectionModel *theSelection; //选择模型

    //设置颜色下拉框
    void setColorCombox(QComboBox* combobox);
    //设置图标下拉框
    void setIconCombox(QComboBox* combobox);



    //添加一行
    void addLine(const QString& id, const QString& text, const QColor& color, const QIcon& icon);
    //删除一行
    void delLine(int row){theModel->removeRow(row);};



    void addSign(Sign* sign)
    {
        addLine(sign->id, sign->text, sign->getColor(), sign->getIcon());
        Sign::registerSign(sign->id, sign);
    };
    void delSign(int row)
    {
        QString id = theModel->item(row,0)->text();
        Sign::unregisterSignDev(id);
        delLine(row);
    };

public:
    explicit QFormSignTable(QWidget *parent = nullptr);
    ~QFormSignTable();

private slots:
    void on_pushButton_Add_clicked();

    void on_pushButton_Del_clicked();

    void on_pushButton_SaveExit_clicked();

    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    Ui::QFormSignTable *ui;
};

#endif // QFORMSIGNTABLE_H
