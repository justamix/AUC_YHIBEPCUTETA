#include "onb8.h"
#include "ui_onb8.h"
#include "menuwindow.h"
#include "databasehelper.h"
#include "authenticationmanager.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QFile>
onb8::onb8(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::onb8)
{
    ui->setupUi(this);

    connect(ui->btn1, SIGNAL(clicked(bool)), this, SLOT(add()));
    connect(ui->btnSearch, &QPushButton::clicked, this, &onb8::search);
    connect(ui->btnReport, &QPushButton::clicked, this, &onb8::generateReportRequest);
    AuthenticationManager authenticationManager;
    QString currentUser = authenticationManager.getCurrentUserLogin();
    if(currentUser == "DATAVIEWER")
    {
        ui->btn1->setVisible(false);
    }

    // Вызываем метод selectAll() через таймер с нулевой задержкой
    QTimer::singleShot(0, this, SLOT(selectAll()));

    // Устанавливаем количество столбцов в таблице
    ui->tw->setColumnCount(2); // Измените на количество столбцов в вашей таблице

    // Задаем заголовки столбцов таблицы
    ui->tw->setHorizontalHeaderItem(0, new QTableWidgetItem("Преподаватель"));
    ui->tw->setHorizontalHeaderItem(1, new QTableWidgetItem("Дисциплина"));
    // Добавьте заголовки для остальных столбцов, если они есть

    // Устанавливаем растягивание последнего столбца при изменении размера формы
    ui->tw->horizontalHeader()->setStretchLastSection(true);

    // Включаем возможность прокрутки содержимого таблицы
    ui->tw->setAutoScroll(true);

    // Устанавливаем режим выделения ячеек: только одна строка
    ui->tw->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tw->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Разрешаем пользователю сортировать данные по столбцам
    //ui->tw->setSortingEnabled(true);
    //ui->tw->sortByColumn(0, Qt::AscendingOrder);

    // Запрещаем редактирование ячеек таблицы
    ui->tw->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Populate combo boxes
    populate1ComboBox();
    populate2ComboBox();
}
onb8::~onb8()
{
    delete ui;
}

void onb8::selectAll()
{

    if (!DatabaseHelper::isDatabaseConnected())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена.");
        return;
    }

    ui->tw->clearContents();

    QString sqlstr = "SELECT t.full_name AS teacher_name, sb.name AS subject_name "
            "FROM teacher_subject ts "
            "JOIN teachers t ON ts.teacher_id = t.id "
            "JOIN subjects sb ON ts.subject_id = sb.id ";

    QSqlQuery query(DatabaseHelper::getDatabaseConnection());
    if (!query.exec(sqlstr))
    {
        QMessageBox::critical(this, "Ошибка", query.lastError().text());
        return;
    }

    ui->tw->setRowCount(query.size());

    int row = 0;
    while (query.next())
    {
        QString subjectName = query.value("subject_name").toString();
        QString teacherName = query.value("teacher_name").toString();
        QTableWidgetItem *subjectItem = new QTableWidgetItem(subjectName);
        QTableWidgetItem *teacherItem = new QTableWidgetItem(teacherName);
        ui->tw->setItem(row, 0, teacherItem);
        ui->tw->setItem(row, 1, subjectItem);
        ++row;
    }

    ui->tw->resizeColumnsToContents();
}

void onb8::add()
{
    QString cmb1 = ui->cmb1->currentText();
    QString cmb2 = ui->cmb2->currentText();
    // Подготавливаем SQL запрос для вставки данных
    QSqlQuery query;
    query.prepare("INSERT INTO teacher_subject (teacher_id, subject_id)"
                  "VALUES ((SELECT id FROM teachers WHERE full_name = :1), "
                  "(SELECT id FROM subjects WHERE name = :2))");

    // Привязываем значения к параметрам запроса
    query.bindValue(":1", cmb1);
    query.bindValue(":2", cmb2);

    // Выполняем SQL запрос для вставки данных
    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        ui->teResult->append("Error: " + query.lastError().text());
        return;
    }

    qDebug() << "Данные успешно добавлены в базу данных.";

    ui->teResult->append("Успех: данные успешно добавлены.");
    selectAll(); // Обновить таблицу, чтобы отобразить новые данные
}
void onb8::populate1ComboBox()
{
    ui->cmb1->clear();
    QSqlQuery query("SELECT id, full_name FROM teachers ORDER BY id");
    while (query.next()) {
        QString pu1 = query.value("id").toString();
        QString pu2 = query.value("full_name").toString();
        ui->cmb1->addItem(pu2, pu1);
    }
}

void onb8::populate2ComboBox()
{
    ui->cmb2->clear();
    QSqlQuery query("SELECT id, name FROM subjects ORDER BY id");
    while (query.next()) {
        QString pu1 = query.value("id").toString();
        QString pu2 = query.value("name").toString();
        ui->cmb2->addItem(pu2, pu1);
    }
}


void onb8::on_btnExit_clicked()
{

    AuthenticationManager authenticationManager;

    // Получаем логин текущего пользователя через созданный объект
    QString currentUser = authenticationManager.getCurrentUserLogin();

    // Получаем список разрешенных кнопок для текущего пользователя
    QStringList allowedButtons = authenticationManager.getAllowedButtons(currentUser);

    // Закрываем текущее окно гостей
    close();

    // Создаем и отображаем новое окно главного меню с передачей списка разрешенных кнопок
    menuwindow *menuWindow = new menuwindow(allowedButtons);
    menuWindow->show();
}

void onb8::search()
{
    // Получаем текст из поля поиска
    QString currentSearchQuery = ui->leSearch->text().trimmed(); // Обрезаем начальные и конечные пробелы

    // Проверяем, пусто ли поле ввода
    if (currentSearchQuery.isEmpty())
    {
        // Если поле ввода пустое, вызываем функцию selectAll для выделения всех строк
        selectAll();
        return; // Завершаем выполнение функции
    }

    // Проверяем, изменилось ли значение поиска с предыдущего запроса
    if (currentSearchQuery != m_lastSearchQuery)
    {
        // Если значение изменилось, обновляем переменную с последним значением поиска
        m_lastSearchQuery = currentSearchQuery;

        // Сбрасываем индекс последнего найденного совпадения
        m_lastFoundIndex = -1;
    }

    // Очищаем выделение в таблице
    ui->tw->clearSelection();

    // Флаг для отслеживания найденных результатов
    bool foundMatch = false;

    // Проходим по всем строкам таблицы и ищем нужную строку
    for (int row = m_lastFoundIndex + 1; row < ui->tw->rowCount(); ++row)
    {
        // Получаем текст в каждой ячейке строки таблицы
        QString rowData;
        for (int col = 0; col < ui->tw->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tw->item(row, col);
            if (item)
                rowData += item->text() + " ";
        }

        // Проверяем, содержит ли текст строки искомую подстроку
        if (rowData.contains(currentSearchQuery, Qt::CaseInsensitive))
        {
            // Выделяем найденную строку
            ui->tw->selectRow(row);

            // Прокручиваем таблицу к найденной строке
            ui->tw->scrollToItem(ui->tw->item(row, 0));

            // Устанавливаем флаг найденного элемента в true
            foundMatch = true;

            // Обновляем индекс последнего найденного совпадения
            m_lastFoundIndex = row;

            // Выходим из цикла, так как строка найдена
            break;
        }
    }

    // Если совпадение не было найдено, выводим сообщение об этом
    if (!foundMatch)
    {
        QMessageBox::information(this, "Поиск", "Больше совпадений не найдено.");
        // Сбрасываем индекс последнего найденного совпадения
        m_lastFoundIndex = -1;
    }
}

void onb8::generateReportRequest()
{
    // Вызываем функцию selectAll для получения данных о комиссиях из таблицы
    selectAll();

    QString reportText = "Запрос на формирование отчета о комиссиях на энергоснабжение:\n\n";
    reportText += "Уважаемые коллеги,\n\n";
    reportText += "Подготовлен отчет о комиссиях на энергоснабжение:\n";

    // Добавляем информацию о каждой комиссии из таблицы
    for (int row = 0; row < ui->tw->rowCount(); ++row) {
        QString periodNumber = ui->tw->item(row, 0)->text();
        QString checkNumbers = ui->tw->item(row, 1)->text();
        reportText += "- В периоде " + periodNumber + " проведены комиссии с номерами: " + checkNumbers + ";\n";
    }

    reportText += "\nС уважением,\n";
    reportText += "Мосэнергосбыт";

    QString fileName = "Energy_Supply_Commission.txt"; // Имя файла для сохранения отчета

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << reportText;
        file.close();
        QMessageBox::information(this, "Успех", "Запрос на формирование отчета о комиссиях на энергоснабжение успешно сохранен в файле " + fileName);

        // После сохранения отчета можно продолжить работу с приложением или завершить его
        // Например, добавив опцию для закрытия диалогового окна или для продолжения работы с приложением
        // QMessageBox::StandardButton reply = QMessageBox::question(this, "Вопрос", "Желаете продолжить работу?", QMessageBox::Yes|QMessageBox::No);
        // if (reply == QMessageBox::No) {
        //     QApplication::quit(); // Закрыть приложение
        // }
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл для сохранения отчета о комиссиях на энергоснабжение.");
    }
}
