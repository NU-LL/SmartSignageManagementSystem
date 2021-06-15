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

private slots:
    void on_pushButton_clicked();

private:
    Ui::QFormOptions *ui;
};

#endif // QFORMOPTIONS_H
