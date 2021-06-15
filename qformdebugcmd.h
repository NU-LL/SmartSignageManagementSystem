#ifndef QFORMDEBUGCMD_H
#define QFORMDEBUGCMD_H

#include <QWidget>

namespace Ui {
class QFormDebugCmd;
}

class QFormDebugCmd : public QWidget
{
    Q_OBJECT

public:
    explicit QFormDebugCmd(QWidget *parent = nullptr);
    ~QFormDebugCmd();

private slots:
    void on_pushButton_set_Voice_clicked();

    void on_pushButton_set_Lightness_clicked();

private:
    Ui::QFormDebugCmd *ui;
};

#endif // QFORMDEBUGCMD_H
