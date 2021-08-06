#ifndef QFORMOPTIONS_H
#define QFORMOPTIONS_H

#include <QWidget>

namespace Ui {
class QFormOptions;
}

class QFormOptions : public QWidget
{
    Q_OBJECT

public:
    explicit QFormOptions(QWidget *parent = nullptr);
    ~QFormOptions();

    static QString ConfigFilePath;
    static QString IconPath;
    static bool defaultStartServer;

    static void Init();
    static bool save();

private slots:
    void on_pushButton_ConfigFile_clicked();

    void on_pushButton_Icon_clicked();

    void on_checkBox_stateChanged(int arg1);

private:
    Ui::QFormOptions *ui;
};

#endif // QFORMOPTIONS_H
