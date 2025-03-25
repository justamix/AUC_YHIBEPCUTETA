#ifndef MENUWINDOW_H
#define MENUWINDOW_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include "ui_menuwindow.h"

class menuwindow : public QDialog
{
    Q_OBJECT

public:
    explicit menuwindow(const QStringList &allowedButtons, QWidget *parent = nullptr);
    ~menuwindow();

private slots:
    void onb11();
    void onb22();
    void onb33();
    void onb44();
    void onb55();
    void onb66();
    void onb77();
    void onb88();
    void onExitButtonClicked();
    void onReturnToRegistrationClicked();
private:
    Ui::menuwindow *ui;
    QPushButton *exitButton;
    QPushButton *returnToRegistrationButton;
};

#endif // MENUWINDOW_H
