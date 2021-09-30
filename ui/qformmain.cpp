#include "ui/qformmain.h"
#include "ui_qformmain.h"

#include "config.h"


#include <QDebug>
#include <QStandardItemModel>
#include <QItemSelectionModel>

#include <QTimer>
#include <QRegExp>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>

#include "protocol/protocol.h"
#include "ui/mainwindow.h"
#include "qdialogsetsigndev.h"
#include "qformsigntable.h"
#include "qdialogadddev.h"


extern MainWindow* mainWindow;



QFormMain::QFormMain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFormMain)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时自动删除

    theModel = new QStandardItemModel(0, 0, this); //数据模型
    theSelection = new QItemSelectionModel(theModel);//选择模型

    ui->tableView->setModel(theModel); //设置数据模型
    ui->tableView->setSelectionModel(theSelection); //设置选择模型

    //设置代理
//    buttonDelegate buttondelegate;
//    ui->tableView->setItemDelegateForColumn(3, &buttondelegate);

    //初始化配置文件
    QFormOptions::Init();
    //初始化标示语表
    Sign::Init();
    //初始化分组
    Group::Init();
    //初始化设备表
    TcpSignDevice::Init();



    //加载表格
//    theModel->setHorizontalHeaderLabels({"id", "组名", "名称", "标示语", "网络状态", "语音设置", "闪光灯设置", "警示设置", "设备控制"}); //设置表头文字
    theModel->setHorizontalHeaderLabels({"id", "组名", "名称", "标示语", "警示语", "网络状态", "语音状态", "闪光灯状态", "警示状态", "异常与故障状态"}); //设置表头文字
    foreach(TcpSignDevice* dev, TcpSignDevice::DeviceTable)
        dev->item = addLine(dev);

    //tcp server
    TcpServer& server = TcpServer::getHandle();
    //连接tcp信号槽
    connect(&server, static_cast<void (TcpServer:: *)(int, void*)>(&TcpServer::message),
            this, static_cast<void (QFormMain:: *)(int, void*)>(&QFormMain::recMessage));

    //安装过滤器 捕获搜索快捷键
    installEventFilter(this);
}

QFormMain::~QFormMain()
{
    delete ui;
}



//消息接收槽函数
void QFormMain::recMessage(int type, void* message)
{
    if(message == nullptr)
        return;

    switch(type)
    {
//        case(MESSAGE_DISCONNECTION)://断开设备
        case(MESSAGE_CHANGE_STATUS)://刷新设备
        {
            TcpSignDevice* dev = static_cast<TcpSignDevice*>(message);
            if(dev->info == dev->id)//新建立的临时对象无需发送断连消息
                break;
//            dev->offline = 0;//在线
            refreshGui(dev);
            qDebug() << "改变状态" << dev->id << " : " << dev->name << " : " << dev->info;
            break;
        }
        case(MESSAGE_ADDDEVICE)://添加新设备
        {
            TcpSignDevice* dev = static_cast<TcpSignDevice*>(message);
            dev->offline = 0;//在线
            MainWindow::showMessageBox(QMessageBox::Information, "通知", "新设备："+dev->name+" 上线", 2000);
            addDevice(dev);
            qDebug() << "添加新设备" << dev->id << " : " << dev->name << " : " << dev->info;
            break;
        }
    }
}




//设置分组下拉框
void QFormMain::setGroupCombox(QComboBox *combobox)
{
    combobox->addItems(Group::getGroupNameList());
}

//设置警示语下拉框
void QFormMain::setSignCombox(QComboBox *combobox)
{
//    auto setCmbColor = [](QComboBox *combobox, const QColor color){
//        QPalette plt = combobox->palette();
//        plt.setColor(QPalette::Text, color);
//        combobox->setPalette(plt);
//    };

    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(combobox->model());
    int i = 0;
    foreach(Sign* sign, Sign::getSignTable())
    {
        combobox->addItem(sign->getIcon(), sign->text, QVariant::fromValue(sign));//添加图标 文字 自定义对象
        pItemModel->item(i++)->setForeground(sign->getColor());//修改某项文本颜色
    }

    //设置颜色
//    QStandardItemModel *pItemModel = qobject_cast<QStandardItemModel*>(combobox->model());
//    for (int i = 0; i < combobox->count(); i++)
//    {
//        Sign* sign = combobox->itemData(i).value<Sign*>();
//        pItemModel->item(i)->setForeground(sign->getColor());//修改某项文本颜色
////        pItemModel->item(i)->setBackground(sign.getColor());//修改某项背景颜色
//    }
////    for (int i = 0; i < combobox->count(); i++) {
//////            pItemModel->item(i)->setIcon(QIcon(":/TestComboBox/Resources/deletered.png"));    //修改某项图标
//////            pItemModel->item(i)->setText("修改的文本  " + QString::number(i + 1));                          //修改某项文本
////        QString str = pItemModel->item(i)->text();
////        pItemModel->item(i)->setForeground(QColor(255, 0, 0));            //修改某项文本颜色
////        pItemModel->item(i)->setBackground(QColor(220,220,220));    //修改某项背景颜色
//////            pItemModel->item(i)->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);    //修改某项文本对齐方式
////    }
////    combobox->setStyleSheet("QComboBox{background:black}");
//    sign.color = combobox->currentData().toString().toUShort(nullptr, 16);
//    combobox->setStyleSheet(QString("QComboBox{background:%1}").arg(sign.getStrColor()));

    Sign* sign = combobox->currentData().value<Sign*>();
    combobox->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));
    connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        //先获取信号的发送者
        QComboBox *cmb = qobject_cast<QComboBox *>(sender());
        Sign* sign = cmb->itemData(index).value<Sign*>();
        cmb->setStyleSheet(QString("QComboBox{color:%1;}").arg(sign->getStrColor()));

        //设置字体颜色
//        QPalette plt = cmb->palette();
//        static QColor color;
//        color = sign->getColor();
//        plt.setColor(QPalette::Text, QColor(150, 10, 55));
//        cmb->setPalette(plt);
    });
}

//设置开关选项下拉框
void QFormMain::setSwitchCombox(QComboBox *combobox)
{
    combobox->addItems({"关","开"});
}



QStandardItem *QFormMain::addLine(const TcpSignDevice *dev)
{
    //排除意外情况
    if(dev == nullptr)
    {
        QMessageBox::warning(this, tr("警告"), "传入指针无效");
        return nullptr;
    }
    Sign* sign = Sign::findSign(dev->signid);
    if(sign == nullptr)
    {
        QMessageBox::warning(this, tr("警告"), QString("未找到该标示语id：")+dev->signid+", 插入设备错误");
        return nullptr;
    }
    Sign* warn_sign = Sign::findSign(dev->warnid);
    if(warn_sign == nullptr)
    {
        QMessageBox::warning(this, tr("警告"), QString("未找到该警示语id：")+dev->warnid+", 插入设备错误");
        return nullptr;
    }
    //在表格最后添加行
    QList<QStandardItem*> aItemList; //容器类
    QStandardItem *aItem;
    QStandardItem *res;

    //id
    aItem=new QStandardItem(dev->id);
    res = aItem;
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //组名
    aItem=new QStandardItem(dev->groupname);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //标识牌名称
    aItem=new QStandardItem(dev->name);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //标示语
    aItem=new QStandardItem(sign->text);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItem->setForeground(sign->getQtColor());//设置字体颜色
    aItem->setIcon(sign->getIcon());//设置图标
    aItem->setData(QVariant::fromValue((Sign *)sign));//存放标示语的指针
    aItemList << aItem;
    //警示语
    aItem=new QStandardItem(warn_sign->text);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItem->setForeground(warn_sign->getQtColor());//设置字体颜色
    aItem->setIcon(warn_sign->getIcon());//设置图标
    aItem->setData(QVariant::fromValue((Sign *)warn_sign));//存放标示语的指针
    aItemList << aItem;
    //网络状态（默认均离线）
    aItem=new QStandardItem();
    if(dev->offline)
    {
        aItem->setText("离线");
        aItem->setIcon(icon_offline);//设置图标
    }else
    {
        aItem->setText("在线");
        aItem->setIcon(icon_online);//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //语音设置
    aItem=new QStandardItem();
    if(dev->voice)
    {
        aItem->setText("开");
        aItem->setIcon(icon_open);//设置图标
    }else
    {
        aItem->setText("关");
        aItem->setIcon(icon_close);//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //闪光灯设置
    aItem=new QStandardItem();
    if(dev->flash)
    {
        aItem->setText("开");
        aItem->setIcon(icon_open);//设置图标
    }else
    {
        aItem->setText("关");
        aItem->setIcon(icon_close);//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //警示设置
    aItem=new QStandardItem();
    if(dev->alert)
    {
        aItem->setText("开");
        aItem->setIcon(icon_open);//设置图标
    }else
    {
        aItem->setText("关");
        aItem->setIcon(icon_close);//设置图标
    }
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
    aItemList << aItem;
    //报警状态
    QString sta;
    QBrush brush = QBrush(Qt::white);
    if(dev->stafault)
    {
        if(dev->fault.people_approach)
        {
            sta += "人员靠近 ";
            brush = QBrush(Qt::darkRed);
        }
        if(dev->fault.Power)
        {
            sta += "电源故障 ";
            brush = QBrush(Qt::darkBlue);
        }
        if(dev->fault.controller)
        {
            sta += "控制器故障 ";
            brush = QBrush(Qt::darkCyan);
        }
        if(dev->fault.power_off)
        {
            sta += "交流电断电 ";
            brush = QBrush(Qt::darkMagenta);
        }
        if(dev->fault.low_battery)
        {
            sta += "电池电量过低 ";
            brush = QBrush(Qt::darkYellow);
        }
        if(dev->fault.manual_configuration)
        {
            sta += "红外遥控手动配置中 ";
            brush = QBrush(Qt::darkGray);
        }
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", dev->name+":"+sta, 2000);
    }else
        sta = "无异常";
    aItem=new QStandardItem(sta);
    aItem->setTextAlignment(Qt::AlignCenter);
    aItem->setEditable(false);//禁止编辑
//    aItem->setForeground(brush);//设置文字颜色
//    theModel->setData(aItem->index(), brush, Qt::BackgroundColorRole);//设置颜色
    aItemList << aItem;

    //添加到新的一行
    theModel->insertRow(theModel->rowCount(), aItemList); //插入一行，需要每个Cell的Item
    theModel->setData(aItem->index(), brush, Qt::BackgroundColorRole);//设置状态栏颜色
    QModelIndex curIndex = theModel->index(theModel->rowCount()-1, 0);//创建最后一行的ModelIndex
    theSelection->clearSelection();//清空选择项
    theSelection->setCurrentIndex(curIndex, QItemSelectionModel::Select);//设置刚插入的行为当前选择行

    return res;
}


//更新 SignDevice 中的数据到表格中
QBitArray QFormMain::refreshGui(TcpSignDevice *dev)
{
    QBitArray res(16, 0);
    qint8 colidx = 1;
    Sign *sign = Sign::findSign(dev->signid);
    if(sign == nullptr)
    {
        MainWindow::showMessageBox(QMessageBox::Critical, "错误", "找不到对应的标示语id："+dev->signid+" 刷新数据无效");
        return res;
    }
    Sign *warn_sign = Sign::findSign(dev->warnid);
    if(warn_sign == nullptr)
    {
        MainWindow::showMessageBox(QMessageBox::Critical, "错误", "找不到对应的警示语id："+dev->warnid+" 刷新数据无效");
        return res;
    }
    if(dev->item == nullptr)
    {
        MainWindow::showMessageBox(QMessageBox::Warning, "错误", "item指针无效，刷新数据无效");
        return res;
    }


    res[colidx] = modifyCell(dev->item->row(), colidx, dev->groupname);
    colidx++;
    res[colidx] = modifyCell(dev->item->row(), colidx, dev->name);
    colidx++;
    //设置标示语
    QStandardItem* signItem = theModel->item(dev->item->row(), colidx);//标示语栏
    Sign * lastsign = signItem->data().value<Sign*>();//取出上次的指针
    if(lastsign != sign)
    {
        //更新标示语
        res[colidx] = true;
        signItem->setForeground(sign->getColor());//设置字体颜色
        signItem->setIcon(sign->getIcon());//设置图标
        signItem->setText(sign->text);
        //修改自定义数据 放入最新指针
        signItem->setData(QVariant::fromValue((Sign *)sign));//存放标示语的指针
    }
    colidx++;
    //设置警示语
    QStandardItem* warnsignItem = theModel->item(dev->item->row(), colidx);//警示语栏
    Sign * warnlastsign = warnsignItem->data().value<Sign*>();//取出上次的指针
    if(warnlastsign != warn_sign)
    {
        //更新警示语
        res[colidx] = true;
        warnsignItem->setForeground(warn_sign->getColor());//设置字体颜色
        warnsignItem->setIcon(warn_sign->getIcon());//设置图标
        warnsignItem->setText(warn_sign->text);
        //修改自定义数据 放入最新指针
        warnsignItem->setData(QVariant::fromValue((Sign *)warn_sign));//存放标示语的指针
    }
    colidx++;

    if(dev->offline)
        res[colidx] = modifyCell(dev->item->row(), colidx, "离线", icon_offline);
    else
        res[colidx] = modifyCell(dev->item->row(), colidx, "在线", icon_online);
    colidx++;

    if(dev->voice)
        res[colidx] = modifyCell(dev->item->row(), colidx, "开", icon_open);
    else
        res[colidx] = modifyCell(dev->item->row(), colidx, "关", icon_close);
    colidx++;

    if(dev->flash)
        res[colidx] = modifyCell(dev->item->row(), colidx, "开", icon_open);
    else
        res[colidx] = modifyCell(dev->item->row(), colidx, "关", icon_close);
    colidx++;

    if(dev->alert)
        res[colidx] = modifyCell(dev->item->row(), colidx, "开", icon_open);
    else
        res[colidx] = modifyCell(dev->item->row(), colidx, "关", icon_close);
    colidx++;


    //设置报警状态
    QString sta;
    QBrush brush = QBrush(Qt::white);
    if(dev->stafault)
    {
        if(dev->fault.people_approach)
        {
            sta += "人员靠近 ";
            brush = QBrush(Qt::darkRed);
        }
        if(dev->fault.Power)
        {
            sta += "电源故障 ";
            brush = QBrush(Qt::darkBlue);
        }
        if(dev->fault.controller)
        {
            sta += "控制器故障 ";
            brush = QBrush(Qt::darkCyan);
        }
        if(dev->fault.power_off)
        {
            sta += "交流电断电 ";
            brush = QBrush(Qt::darkMagenta);
        }
        if(dev->fault.low_battery)
        {
            sta += "电池电量过低 ";
            brush = QBrush(Qt::darkYellow);
        }
        if(dev->fault.manual_configuration)
        {
            sta += "红外遥控手动配置中 ";
            brush = QBrush(Qt::darkGray);
        }
    }else
        sta = "无异常";
    res[colidx] = modifyCell(dev->item->row(), colidx, sta, QIcon(), brush);
    if(res[colidx] && dev->stafault)//如果修改了 且 有异常 则需要提醒
    {
        MainWindow::showMessageBox(QMessageBox::Warning, "警告", dev->name+":"+sta, 2000);
    }
    return res;
}

//更新表格并发送数据更新下位机
void QFormMain::refresh(TcpSignDevice* dev)
{
    if(dev == nullptr)
    {
        qDebug() << "TcpSignDevice*为空 刷新错误";
        return ;
    }

    //刷新表格数据
    QBitArray res = refreshGui(dev);
    //修改相关设置 需要发送给下位机
    for(int i = 1; i < res.size(); i++)//第0项为id 不可能修改
    {
        if(res[i])//该项有改动
        {
            //todo 发送信息给下位机 通知设置已经更改
            //todo 这里只记录了三个check状态 其他改变未记录
            qDebug() << QString("第")+i+"项有改动";

        }
    }
    //有修改 同时下位机在线 则给下位机发送数据
    //注意：这里会统计所有有修改的数量，但是该处由于 QDialogSetSignDev 的限制 能修改的只有
    //      名字、标示语、三个开关量，而这些改变均需要通知下位机。其他如id、是否在线、报警状态，
    //      在这里则不可能修改。所以用 res.count 是没问题的
    if(res.count(true) != 0 && dev->offline == 0)
    {
        Sign *sign = Sign::findSign(dev->signid);
        Sign *warn_sign = Sign::findSign(dev->warnid);
        //发送信息

        QByteArray data;

        //发送标示语信息
        if(res[3])//只有当标示语变更时才发送文本信息
        {
            data += QString("%1").arg(sign->id, 3, QLatin1Char('0')).toLocal8Bit();//提示语编号
            data += sign->text.toLocal8Bit();//标示语内容
            dev->sendMessage(00, 12, data);//发送标示语文本信息
            data.clear();

            if(!sign->iconfilename.isEmpty())//有图标才判断
            {
                //获得下位机图标列表
                dev->sendMessage(00, 22);//获得下位机列表
                if(dev->images.contains(sign->geticonbasename()) == false)//存在图标 且 下位机中不包含当前图标
                {
                    qDebug() << "准备更新下位机警示语图标：" << sign->iconfilename;
                    //发送新图标
                    dev->sendFile(sign->getIconAbsolutePath());
                }
            }
        }

        //发送警示语信息（由于亮度、音量、报警时间没有确定是否修改 所以 02 指令一直发送）
        data += dev->stabyte & 0x07 + '0';//模式，即状态字 无需考虑是否离线
        data += warn_sign->color;//颜色
        data += dev->light;//亮度
        data += dev->vol;//音量
        data += dev->delay;//报警时间
        data += QString("%1").arg(warn_sign->id, 4, QLatin1Char('0')).toLocal8Bit();//提示语编号
        data += warn_sign->geticonbasename().rightJustified(3, '0').left(3).toLocal8Bit();//图片编号
        dev->sendMessage(00, 02, data);//发送警示语信息
        data.clear();

        if(res[4])//只有当警示语变更时才尝试判断是否需要发送图片
        {
            if(!warn_sign->iconfilename.isEmpty())//有图标才判断
            {
                //获得下位机图标列表
                dev->sendMessage(00, 22);//获得下位机列表
                if(dev->images.contains(warn_sign->geticonbasename()) == false)//存在图标 且 下位机中不包含当前图标
                {
                    qDebug() << "准备更新下位机标示语图标：" << warn_sign->iconfilename;
                    //发送新图标
                    dev->sendFile(warn_sign->getIconAbsolutePath());
                }
            }
        }

        //发送名字与安装位置
        if(res[2])//修改名称
        {
            data += QString("%1").arg(dev->name, -16, QLatin1Char(0)).toLocal8Bit();//名称
            data += QString("%1").arg("", -16, QLatin1Char(0)).toLocal8Bit();//安装位置
            dev->sendMessage(00, 71, data);//发送警示牌名称
            data.clear();
        }
    }
}


//事件截获
bool QFormMain::eventFilter(QObject *watched, QEvent *event)
{
    if(nullptr != event && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if((keyEvent->key() == Qt::Key_F) && (keyEvent->modifiers() == Qt::ControlModifier))//Ctrl+F 搜索
        {
//            qDebug() << "Ctrl+F";
            bool ok=false;
            QString searchname = QInputDialog::getText(this, tr("搜索标示语名称"),
                                                 tr("请输入待搜索的标示语名称："), QLineEdit::Normal,
                                                 QString(""), &ok);
            //只有当用户输入不为空且不在列表中存在时才插入
            if (ok && !searchname.isEmpty())
            {
                bool sta = false;
                foreach(TcpSignDevice* dev, TcpSignDevice::DeviceTable)
                {
                    if(dev->name == searchname)
                    {
                        sta = true;
                        ui->tableView->selectRow(dev->item->row());//光标选中该行
//                        theSelection->setCurrentIndex(signdev->item->index(), QItemSelectionModel::Select);
                        break;
                    }
                }
                if(!sta)
                    QMessageBox::information(this, "通知", "没有找到标示牌："+searchname);
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

//关闭窗口信号
void QFormMain::closeEvent(QCloseEvent *event)
{
    QMessageBox::warning(this, "警告", "该窗口禁止关闭");
    event->ignore();  //忽略退出信号，程序继续运行
}





//添加设备
void QFormMain::on_pushButton_AddDevice_clicked()
{
    //弹窗
    QDialogAddDev *dlgAddDev = new QDialogAddDev(this);
    int ret = dlgAddDev->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)//OK按钮被按下
    {
        if(dlgAddDev->getDeviceId().isEmpty())
        {
            QMessageBox::warning(this, "警告", "添加失败（id为空），请输入id");
            delete dlgAddDev;
            return ;
        }
        TcpSignDevice* dev = new TcpSignDevice(nullptr, dlgAddDev->getDeviceId(), dlgAddDev->getName(), (quint8)0x08);//离线
        dev->groupname = dlgAddDev->getGroupName();
        dev->signid = dlgAddDev->getSignIdx();
        dev->warnid = dlgAddDev->getWarnSignIdx();
        //注册
        TcpSignDevice::DeviceTable[dlgAddDev->getDeviceId()] = dev;
        addDevice(dev);
    }
    delete dlgAddDev;
}

//删除设备
void QFormMain::on_pushButton_DelDevice_clicked()
{
    QModelIndexList curIndexList = theSelection->selectedRows();//获取模型索引
    if(curIndexList.isEmpty())
    {
        QMessageBox::information(this, "通知", "请选择对应的标识牌");
        return ;
    }
    for(int idx = 0; idx < curIndexList.size(); )
    {
        QModelIndex& index = curIndexList[idx];
        if(index.isValid())
        {
            //确认是否删除
            QString id = theModel->data(theModel->index(index.row(), 0)).toString();//获得该行的id
            TcpSignDevice* dev = TcpSignDevice::find(id);
            if(dev == nullptr)
            {
                QMessageBox::warning(this, "警告", QString("设备id[%1]异常，删除失败").arg(id));
                continue;
            }
            if(QMessageBox::No == QMessageBox::question(this, "确认删除", QString("确认删除设备：%1[%2]？%3")
                                              .arg(dev->name, dev->id, (dev->offline == 0)?QString(" 该设备仍在线[%1]").arg(dev->info):"")))
            {
                idx++;//不删除当前项 则跳到下一项
                continue;
            }
            //开始删除
            delDevice(index.row());
        }
        curIndexList = theSelection->selectedRows();//更新索引（删除操作会导致索引变化）
    }


//    //删除行
//    QModelIndex curIndex=theSelection->currentIndex();//获取模型索引
//    if(!curIndex.isValid())
//    {
//        QMessageBox::information(this, "通知", "列表为空，不能再删除");
//        return ;
//    }
//    //确认是否删除
//    QString id = theModel->data(theModel->index(curIndex.row(), 0)).toString();//获得该行的id
//    TcpSignDevice* dev = TcpSignDevice::find(id);
//    if(dev == nullptr)
//    {
//        QMessageBox::warning(this, "警告", "设备id异常，删除失败");
//        return ;
//    }
//    if(QMessageBox::No == QMessageBox::question(this, "确认删除", QString("确认删除设备：%1[%2]？%3")
//                                      .arg(dev->name, dev->id, (dev->offline == 0)?QString(" 该设备仍在线[%1]").arg(dev->info):"")))
//    {
//        return ;
//    }
//    //开始删除
//    int counts = theModel->rowCount();
//    delDevice(curIndex.row());

    //如果不是删除最后一行 则重新设置当前选择行
//    if(curIndex.row () != counts - 1)
//        theSelection->setCurrentIndex (curIndex, QItemSelectionModel::Select);
}

//保存设备列表
void QFormMain::on_pushButton_SaveTable_clicked()
{
    //确保id唯一
    QSet<QString> _set;
    for (int i = 0; i < theModel->rowCount(); i++)
    {
        QString id = theModel->item(i,0)->text();
        _set.insert(id);
    }
    if(_set.size() != theModel->rowCount())
    {
        QMessageBox::warning(this, tr("警告"), tr("保存设备列表失败（请确保id唯一）"));
        return;
    }

    //保存数据
    TcpSignDevice::save();

    QMessageBox::information(this, tr("通知"), tr("保存设备列表成功"));
}

//标示语表
void QFormMain::on_pushButton_SignTable_clicked()
{
    QFormSignTable *formSignTable = new QFormSignTable(this);
    formSignTable->setWindowFlag(Qt::Window,true);
    formSignTable->setWindowModality(Qt::WindowModal);//窗口阻塞，但是该函数不会阻塞
    formSignTable->show();
}





void QFormMain::on_tableView_clicked(const QModelIndex &index)
{
    Q_UNUSED(index);
//    QAbstractItemModel *Imodel=ui->tableView->model();
    //更换标示语
//    if(index.column() == 8)
//    {
//        if(QMessageBox::question(mainWindow, tr("提示"), tr("是否更换标示语")) == QMessageBox::Yes)
//        {
//            //更换
//        }else
//        {
//            //不更换
//        }
//    }
//    QModelIndex Iindex = Imodel->index(index.row(),1);//index.row()为算选择的行号。1为所选中行的第一列。。
//    QVariant datatemp=Imodel->data(Iindex);
//    QString name=datatemp.toString();//name即为所选择行的第一列的值。。。
}



//双击修改对象
void QFormMain::on_tableView_doubleClicked(const QModelIndex &index)
{
    //获得该行对应的对象
    QString id = theModel->data(theModel->index(index.row(), 0)).toString();//获得该行的id
    TcpSignDevice* dev = TcpSignDevice::find(id);
    if(dev == nullptr)
        return ;

    //弹窗
    QDialogSetSignDev *dlgSignDev = new QDialogSetSignDev(this, QDialogSetSignDev::DEFAULT_MODE);
    dlgSignDev->setParameters(dev);//预先设置
    int ret = dlgSignDev->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)//OK按钮被按下
    {
        refresh(dev);
    }
    delete dlgSignDev;
}



//========================批量设置===================================


void QFormMain::batchSet(int mode)
{
    //获得光标
    QModelIndexList curIndexList = theSelection->selectedRows();//获取模型索引
    if(curIndexList.isEmpty())
    {
        QMessageBox::information(this, "通知", "请选择对应的标识牌");
        return ;
    }
//    qDebug() << curIndexList;



    //获得该行对应的对象
    QString id = theModel->data(theModel->index(curIndexList[0].row(), 0)).toString();//获得索引第一行的id
    TcpSignDevice* templateDev = TcpSignDevice::find(id);//用该列对应的设备作为模板
    if(templateDev == nullptr)
    {
        QMessageBox::warning(this, "警告", QString("模板设备id[%1]异常，更新失败").arg(id));
        return ;
    }
    //弹窗
    QDialogSetSignDev *dlgSignDev = new QDialogSetSignDev(this, static_cast<QDialogSetSignDev::mode>(mode));//批量设置模式
    dlgSignDev->setParameters(templateDev);//预先设置
    dlgSignDev->setWindowTitle("批量设置");
    int ret = dlgSignDev->exec();// 以模态方式显示对话框
    if (ret == QDialog::Accepted)//OK按钮被按下
    {
        //更新每个设备的状态
        for(int idx = 0; idx < curIndexList.size(); idx++)
        {

            QModelIndex& index = curIndexList[idx];
            if(index.isValid())
            {
                //确认是否删除
                id = theModel->data(theModel->index(index.row(), 0)).toString();//获得该行的id
                TcpSignDevice* dev = TcpSignDevice::find(id);//重新查找每行对应的设备
                if(dev == nullptr)
                {
                    QMessageBox::warning(this, "警告", QString("设备id[%1]异常，更新失败").arg(id));
                    continue;
                }
                MainWindow::showStatusText(QString("正在更新第%1/%2个设备(%3[%4])，更新中请勿进行其他操作")
                                           .arg(idx+1)
                                           .arg(curIndexList.size())
                                           .arg(dev->name, dev->id), 0);//在主界面的状态栏显示信息 永久显示
                //只有在线才更新状态
                if(dev->offline == 0)
                {
                    //将模板的一些参数赋值到当前行
                    if(idx != 0)//第一行是模板 无需修改
                    {
                        switch(mode)
                        {
                            case(QDialogSetSignDev::BATCH_MODE):
                            {
                                dev->groupname = templateDev->groupname;
                                dev->signid = templateDev->signid;
                                dev->warnid = templateDev->warnid;
                                dev->alert = templateDev->alert;
                                dev->flash = templateDev->flash;
                                dev->voice = templateDev->voice;
                                dev->vol = templateDev->vol;
                                dev->light = templateDev->light;
                                dev->delay = templateDev->delay;
                                break;
                            }
                            case(QDialogSetSignDev::SIGN_MODE):
                            {
                                dev->signid = templateDev->signid;
                                break;
                            }
                            case(QDialogSetSignDev::GROUP_MODE):
                            {
                                dev->groupname = templateDev->groupname;
                                break;
                            }
                            case(QDialogSetSignDev::VOICE_MODE):
                            {
                                dev->voice = templateDev->voice;
                                break;
                            }
                            case(QDialogSetSignDev::FLASH_MODE):
                            {
                                dev->flash = templateDev->flash;
                                break;
                            }
                            case(QDialogSetSignDev::ALERT_MODE):
                            {
                                dev->alert = templateDev->alert;
                                break;
                            }
                        }
                    }
                    refresh(dev);
                }
            }
        }
    }
    MainWindow::showStatusText("批量设置成功");//在主界面的状态栏显示信息 永久显示
    delete dlgSignDev;
}



//语音设置
void QFormMain::on_pushButton_VoiceSet_clicked()
{
    batchSet(QDialogSetSignDev::VOICE_MODE);
}

//闪光设置
void QFormMain::on_pushButton_FlashSet_clicked()
{
    batchSet(QDialogSetSignDev::FLASH_MODE);
}

//警示设置
void QFormMain::on_pushButton_AlarmSet_clicked()
{
    batchSet(QDialogSetSignDev::ALERT_MODE);
}

//更换标示语设置
void QFormMain::on_pushButton_ChangeAlarm_clicked()
{
    batchSet(QDialogSetSignDev::SIGN_MODE);
}

//批量全部设置
void QFormMain::on_pushButton_BatchSet_clicked()
{
    batchSet(QDialogSetSignDev::BATCH_MODE);
}

