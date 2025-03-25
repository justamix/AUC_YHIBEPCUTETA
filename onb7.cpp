#include "onb7.h"
#include "ui_onb7.h"
#include "menuwindow.h"
#include "databasehelper.h"
#include "authenticationmanager.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QFile>
onb7::onb7(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::onb7)
{
    ui->setupUi(this);

    connect(ui->btn1, SIGNAL(clicked(bool)), this, SLOT(add()));
    connect(ui->btn2, SIGNAL(clicked(bool)), this, SLOT(remove()));
    connect(ui->btnSearch, &QPushButton::clicked, this, &onb7::search);
    connect(ui->btnReport, &QPushButton::clicked, this, &onb7::generateReportRequest);
    connect(ui->btnFilter, SIGNAL(clicked(bool)), this, SLOT(filter()));
    AuthenticationManager authenticationManager;
    QString currentUser = authenticationManager.getCurrentUserLogin();
    if(currentUser == "DATAVIEWER")
    {
        ui->btn1->setVisible(false); // Скрыть кнопку 1
        ui->btn2->setVisible(false);
    }

    // Вызываем метод selectAll() через таймер с нулевой задержкой
    QTimer::singleShot(0, this, SLOT(selectAll()));

    // Устанавливаем количество столбцов в таблице
    ui->tw->setColumnCount(5); // Измените на количество столбцов в вашей таблице

    // Задаем заголовки столбцов таблицы
    ui->tw->setHorizontalHeaderItem(0, new QTableWidgetItem("id Зачета"));
    ui->tw->setHorizontalHeaderItem(1, new QTableWidgetItem("Студент"));
    ui->tw->setHorizontalHeaderItem(2, new QTableWidgetItem("Дисциплина"));
    ui->tw->setHorizontalHeaderItem(3, new QTableWidgetItem("Преподаватель"));
    ui->tw->setHorizontalHeaderItem(4, new QTableWidgetItem("Оценка"));
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
    populate3ComboBox();
    populate4ComboBox();
}

onb7::~onb7()
{
    delete ui;
}

void onb7::selectAll()
{

    if (!DatabaseHelper::isDatabaseConnected())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена.");
        return;
    }

    ui->tw->clearContents();

    QString sqlstr = "SELECT e.id, s.full_name AS student_name, sb.name AS subject_name, t.full_name AS teacher_name, e.grade "
                     "FROM exam e "
                     "JOIN students s ON e.student_id = s.student_id "
                     "JOIN subjects sb ON e.subject_id = sb.id "
                     "JOIN teachers t ON e.teacher_id = t.id";

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
        QString id = query.value("id").toString();
        QString studentName = query.value("student_name").toString();
        QString subjectName = query.value("subject_name").toString();
        QString teacherName = query.value("teacher_name").toString();
        QString grade = query.value("grade").toString();

        QTableWidgetItem *idItem = new QTableWidgetItem(id);
        QTableWidgetItem *studentItem = new QTableWidgetItem(studentName);
        QTableWidgetItem *subjectItem = new QTableWidgetItem(subjectName);
        QTableWidgetItem *teacherItem = new QTableWidgetItem(teacherName);
        QTableWidgetItem *gradeItem = new QTableWidgetItem(grade);

        ui->tw->setItem(row, 0, idItem);
        ui->tw->setItem(row, 1, studentItem);
        ui->tw->setItem(row, 2, subjectItem);
        ui->tw->setItem(row, 3, teacherItem);
        ui->tw->setItem(row, 4, gradeItem);

        ++row;
    }

    ui->tw->resizeColumnsToContents();
}

void onb7::add()
{
    QString cmb1 = ui->cmb1->currentText();
    QString cmb2 = ui->cmb2->currentText();
    QString cmb3 = ui->cmb3->currentText();
    QString cmb4 = ui->cmb4->currentText();
    // Подготавливаем SQL запрос для вставки данных
    QSqlQuery query;
    query.prepare("INSERT INTO exam (student_id, subject_id, teacher_id, grade)"
                    "VALUES ((SELECT student_id FROM students WHERE full_name = :1), "
                    "(SELECT id FROM subjects WHERE name = :2),"
                    "(SELECT id FROM teachers WHERE full_name = :3), :4)");

    // Привязываем значения к параметрам запроса
    query.bindValue(":1", cmb1);
    query.bindValue(":2", cmb2);
    query.bindValue(":3", cmb3);
    query.bindValue(":4", cmb4);

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



void onb7::remove()
{
    // Проверяем, открыто ли соединение с базой данных
    if (!DatabaseHelper::isDatabaseConnected())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена.");
        return;
    }

    // Получаем текущую выбранную строку
    int curRow = ui->tw->currentRow();

    // Проверяем, что выбранная строка действительно существует
    if (curRow < 0)
    {
        QMessageBox::critical(this, "Ошибка", "Не выбрана строка!");
        return;
    }

    // Просим пользователя подтвердить удаление строки
    if (QMessageBox::question(this, "Удалить", "Удалить строку?", QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel)
        return;

    // Создать объект запроса
    QSqlQuery query(DatabaseHelper::getDatabaseConnection());

    QString pu1 = ui->tw->item(curRow, 0)->text();

    query.prepare("DELETE FROM exam WHERE id = :pupu");
    query.bindValue(":pupu", pu1);

    // Выполняем запрос
    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Ошибка выполнения запроса DELETE");
        return;
    }

    // Выводим сообщение об удалении строки
    ui->teResult->append(QString("Deleted %1 rows").arg(query.numRowsAffected()));

    // Обновляем содержимое компонента таблицы
    selectAll();
}
void onb7::populate1ComboBox()
{
    ui->cmb1->clear();
    QSqlQuery query("SELECT student_id, full_name FROM students ORDER BY student_id");
    while (query.next()) {
        QString pu1 = query.value("student_id").toString();
        QString pu2 = query.value("full_name").toString();
        ui->cmb1->addItem(pu2, pu1);
    }
}

void onb7::populate2ComboBox()
{
    ui->cmb2->clear();
    QSqlQuery query("SELECT id, name FROM subjects ORDER BY id");
    while (query.next()) {
        QString pu1 = query.value("id").toString();
        QString pu2 = query.value("name").toString();
        ui->cmb2->addItem(pu2, pu1);
    }
}

void onb7::populate3ComboBox()
{
    ui->cmb3->clear();
    QSqlQuery query("SELECT id, full_name FROM teachers ORDER BY id");
    while (query.next()) {
        QString pu1 = query.value("id").toString();
        QString pu2 = query.value("full_name").toString();
        ui->cmb3->addItem(pu2, pu1);
    }
}

void onb7::populate4ComboBox()
{
    ui->cmb4->clear();
    int grade;
    for(int i = 1; i <= 4; i++){
        grade = 1 + i;
        ui->cmb4->addItem(QString::number(grade), i);
    }
}

void onb7::on_btnExit_clicked()
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

void onb7::search()
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

void onb7::generateReportRequest()
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
void onb7::filter()
{
    if (!DatabaseHelper::isDatabaseConnected())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена.");
        return;
    }
    QString cmb1 = ui->cmb1->currentText();
    ui->tw->clearContents();

    QString sqlstr = "SELECT e.id, s.full_name AS student_name, sb.name AS subject_name, t.full_name AS teacher_name, e.grade "
                     "FROM exam e "
                     "JOIN students s ON e.student_id = s.student_id "
                     "JOIN subjects sb ON e.subject_id = sb.id "
                     "JOIN teachers t ON e.teacher_id = t.id "
                     "WHERE s.full_name = :cmb1"; // Добавляем условие WHERE для фильтрации по ФИО студента

    QSqlQuery query(DatabaseHelper::getDatabaseConnection());
    query.prepare(sqlstr);
    query.bindValue(":cmb1", cmb1);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", query.lastError().text());
        return;
    }

    ui->tw->setRowCount(query.size());

    int row = 0;
    while (query.next())
    {
        QString id = query.value("id").toString();
        QString studentName = query.value("student_name").toString();
        QString subjectName = query.value("subject_name").toString();
        QString teacherName = query.value("teacher_name").toString();
        QString grade = query.value("grade").toString();

        QTableWidgetItem *idItem = new QTableWidgetItem(id);
        QTableWidgetItem *studentItem = new QTableWidgetItem(studentName);
        QTableWidgetItem *subjectItem = new QTableWidgetItem(subjectName);
        QTableWidgetItem *teacherItem = new QTableWidgetItem(teacherName);
        QTableWidgetItem *gradeItem = new QTableWidgetItem(grade);

        ui->tw->setItem(row, 0, idItem);
        ui->tw->setItem(row, 1, studentItem);
        ui->tw->setItem(row, 2, subjectItem);
        ui->tw->setItem(row, 3, teacherItem);
        ui->tw->setItem(row, 4, gradeItem);

        ++row;
    }

    ui->tw->resizeColumnsToContents();
}

