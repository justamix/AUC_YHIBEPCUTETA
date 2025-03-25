#ifndef ONB8_H
#define ONB8_H

#include <QDialog>

namespace Ui {
class onb8;
}

class onb8 : public QDialog
{
    Q_OBJECT

public:
    explicit onb8(QWidget *parent = nullptr);
    ~onb8();
private slots:
    void on_btnExit_clicked();
    void selectAll();
    void add();
    void search();
    void generateReportRequest();
private:
    Ui::onb8 *ui;
    void populate1ComboBox();
    void populate2ComboBox();
    QString m_lastSearchQuery;
    int m_lastFoundIndex = -1;
};

#endif // ONB7_H
