#ifndef ONB4_H
#define ONB4_H

#include <QDialog>
#include <QTableWidgetItem>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>

namespace Ui {
class onb4;
}

class onb4 : public QDialog
{
    Q_OBJECT

public:
    explicit onb4(QWidget *parent = nullptr);
    ~onb4();
private slots:
    void selectAll();
    void on_btnExit_clicked();
    void add();
    void remove();
    void clean();
    void edit();
    void on_tw_itemSelectionChanged();
    void search();
    void generateReportRequest();
private:
    Ui::onb4 *ui;
    void populate1ComboBox();
    QString m_lastSearchQuery;
    int m_lastFoundIndex = -1;
};

#endif // ONB4_H
