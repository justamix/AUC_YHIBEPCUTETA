#ifndef ONB6_H
#define ONB6_H

#include <QDialog>
#include <QTableWidgetItem>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>


namespace Ui {
class onb6;
}

class onb6 : public QDialog
{
    Q_OBJECT

public:
    explicit onb6(QWidget *parent = nullptr);
    ~onb6();
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
    Ui::onb6 *ui;
    void populate1ComboBox();
    void populate2ComboBox();
    QString m_lastSearchQuery;
    int m_lastFoundIndex = -1;
};

#endif // ONB6_H
