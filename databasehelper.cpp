// databasehelper.cpp

#include "databasehelper.h"
#include <QCoreApplication> // Для доступа к методу addLibraryPath
#include <QSqlDatabase>
#include <QSqlError>
#include "AuthenticationManager.h"
bool DatabaseHelper::databaseConnected = false;
QSqlError DatabaseHelper::lastDbError;


bool DatabaseHelper::connectToDatabase(const QString &hostName, const QString &databaseName,
                                       const QString &userName, const QString &password)
{
    // Добавление путей к драйверам баз данных
    QCoreApplication::addLibraryPath("/opt/homebrew/share/qt/plugins");
    QCoreApplication::addLibraryPath("/usr/local/lib/qt/plugins/sqldrivers");

    // Подключаемся к базе данных
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(hostName);
    db.setDatabaseName(databaseName);
    db.setUserName(userName);
    db.setPassword(password);

    if (db.open()) {
        databaseConnected = true;
        return true;
    } else {
        lastDbError = db.lastError();
        databaseConnected = false;
        return false;
    }
}

bool DatabaseHelper::isDatabaseConnected()
{
    return databaseConnected;
}

QSqlError DatabaseHelper::lastError()
{
    return lastDbError;
}

QSqlDatabase DatabaseHelper::getDatabaseConnection()
{
    // Получаем объект соединения с базой данных
    QSqlDatabase db = QSqlDatabase::database();

    // Проверяем, открыто ли соединение
    if (!db.isOpen()) {
        // Если соединение не открыто, выводим сообщение об ошибке
        qDebug() << "База данных не открыта. Ошибка: " << db.lastError().text();
    }

    return db;
}



bool DatabaseHelper::checkUser(const QString &login, const QString &password)
{

}
