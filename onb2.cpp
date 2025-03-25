#include "onb2.h"
#include "ui_onb2.h"
#include "authenticationmanager.h"
#include "databasehelper.h"
#include "menuwindow.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QDateTime>
#include <QHBoxLayout>
#include <QSqlRecord>
#include <QFile>
#include <QTextStream>
onb2::onb2(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::onb2)
{
    ui->setupUi(this);

    // Подключение сигналов к слотам
    connect(ui->btn1, SIGNAL(clicked(bool)), this, SLOT(add()));
    connect(ui->btn2, SIGNAL(clicked(bool)), this, SLOT(edit()));
    connect(ui->btn3, SIGNAL(clicked(bool)), this, SLOT(remove()));
    connect(ui->btn4, SIGNAL(clicked(bool)), this, SLOT(clean()));
    connect(ui->btnSearch, &QPushButton::clicked, this, &onb2::search);
    connect(ui->btnReport, &QPushButton::clicked, this, &onb2::generateReportRequest);
    AuthenticationManager authenticationManager;
    QString currentUser = authenticationManager.getCurrentUserLogin();
    if(currentUser == "DATAVIEWER")
    {
        ui->btn1->setVisible(false); // Скрыть кнопку 1
        ui->btn2->setVisible(false);
        ui->btn3->setVisible(false);
    }

    // Вызываем метод selectAll() через таймер с нулевой задержкой, чтобы он выполнился после инициализации интерфейса
    QTimer::singleShot(0, this, &onb2::selectAll);

    // Задаем количество столбцов в компоненте таблицы
    ui->tw->setColumnCount(4);

    // Задаем заголовки столбцов таблицы
    ui->tw->setHorizontalHeaderItem(0, new QTableWidgetItem("ID"));
    ui->tw->setHorizontalHeaderItem(1, new QTableWidgetItem("Название"));
    ui->tw->setHorizontalHeaderItem(2, new QTableWidgetItem("Сайт"));
    ui->tw->setHorizontalHeaderItem(3, new QTableWidgetItem("Факультет"));


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

    // Заполнение выпадающих списков
    populate1ComboBox();

}

onb2::~onb2()
{
    delete ui;
}
void onb2::on_btnExit_clicked()
{
    // Создаем объект AuthenticationManager
    AuthenticationManager authenticationManager;
    // Получаем логин текущего пользователя через созданный объект
    QString currentUser = authenticationManager.getCurrentUserLogin();
    // Получаем список разрешенных кнопок для текущего пользователя
    QStringList allowedButtons = authenticationManager.getAllowedButtons(currentUser);
    close();
    // Создаем и отображаем новое окно главного меню с передачей списка разрешенных кнопок
    menuwindow *menuWindow = new menuwindow(allowedButtons);
    menuWindow->show();
}
void onb2::populate1ComboBox()
{
    ui->cmb1->clear();
    QSqlQuery query("SELECT f_name FROM faculties");
    while (query.next()) {
        QString pu1 = query.value("f_name").toString();
        ui->cmb1->addItem(pu1);
    }
}
void onb2::selectAll()
{
    // Проверяем, открыто ли соединение с базой данных
    if (!DatabaseHelper::isDatabaseConnected())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена.");
        return;
    }
    QSqlQuery query(DatabaseHelper::getDatabaseConnection());
    QString sqlstr = "SELECT id, name, website, faculty_id FROM departments ORDER BY id";
    if (!query.exec(sqlstr))
    {
        QMessageBox::critical(this, "Ошибка", query.lastError().text());
        return;
    }
    ui->tw->clearContents();
    ui->tw->setRowCount(0);
    int rowCount = 0;
    while (query.next())
    {
        ui->tw->insertRow(rowCount);
        for (int col = 0; col < ui->tw->columnCount(); ++col)
        {
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setData(Qt::DisplayRole, query.value(col));
            ui->tw->setItem(rowCount, col, item);
        }
        rowCount++;
    }
    ui->tw->resizeColumnsToContents();
}
void onb2::add()
{
    QString pu1 = ui->cmb1->currentText();
    QString pu2 = ui->le2->text();
    QString pu3 = ui->le3->text();
    QSqlQuery query;
    query.prepare("INSERT INTO departments (name, website, faculty_id)"
                    "SELECT :1, :2, f_id FROM faculties WHERE f_name = :3;");
    query.bindValue(":1", pu2);
    query.bindValue(":2", pu3);
    query.bindValue(":3", pu1);
    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        ui->teResult->append("Error: " + query.lastError().text());
        return;
    }
    qDebug() << "Данные успешно добавлены в базу данных.";
    ui->teResult->append("Успех: данные успешно добавлены.");
    selectAll();
}
void onb2::remove()
{
    int curRow = ui->tw->currentRow();
    if (curRow < 0)
    {
        ui->teResult->append("Внимание: строка не выбрана!");
        return;
    }
    QString pu1 = ui->tw->item(curRow, 0)->text();
    // Просим пользователя подтвердить удаление строки
    if (QMessageBox::question(this, "Удалить", "Удалить строку?", QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel)
        return;
    QSqlQuery query;
    query.prepare("DELETE FROM departments WHERE id = :pupu");
    query.bindValue(":pupu", pu1);

    if (!query.exec()) {
        // Проверяем текст ошибки на наличие фразы "foreign key"
        if (query.lastError().text().contains("foreign key", Qt::CaseInsensitive))
        {
            QMessageBox::critical(this, "Ошибка", "Невозможно удалить запись, так как есть связанные данные.");
        }
        else
        {
            // Если запрос не выполнен по другой причине, выводим сообщение об ошибке
            QMessageBox::critical(this, "Ошибка", query.lastError().text());
        }
        return;
    }
    ui->teResult->append("Успех: запись успешно удалена.");
    selectAll();
}
void onb2::edit()
{
    // Получить выбранные значения из выпадающих списков
    QString cmb = ui->cmb1->currentText();
    QString pu1 = ui->le1->text();
    QString pu2 = ui->le2->text();
    QString pu3 = ui->le3->text();
    // Получаем текущую выбранную строку
    int curRow = ui->tw->currentRow();
    // Проверяем, что строка действительно выбрана
    if (curRow < 0)
    {
        ui->teResult->append("Внимание: строка не выбрана!");
        return;
    }
    // Получаем ID организации
    QString fineCode = ui->tw->item(curRow, 0)->text();
    // Подготавливаем SQL запрос для обновления данных
    QSqlQuery query;
    query.prepare("UPDATE departments "
                      "SET name = :a, website = :b, faculty_id = (SELECT f_id FROM faculties WHERE f_name = :c)"
                  "WHERE id = :d");
    query.bindValue(":a", pu2);
    query.bindValue(":b", pu3);
    query.bindValue(":c", cmb);
    query.bindValue(":d", pu1);

    // Выполняем запрос
    if (!query.exec()) {
        ui->teResult->append("Ошибка: " + query.lastError().text());
        return;
    }

    ui->teResult->append("Успех: данные успешно обновлены.");
    selectAll(); // Обновить таблицу после обновления
}

void onb2::search()
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
void onb2::generateReportRequest()
{

    selectAll();

    QString reportText = "Запрос на формирование отчета о Кафедрах:\n\n";
    reportText += "Подготовлен отчет о штрафах:\n";


    for (int row = 0; row < ui->tw->rowCount(); ++row) {
        QString fineCode = ui->tw->item(row, 0)->text();
        QString amount = ui->tw->item(row, 1)->text();
        QString responsibility = ui->tw->item(row, 2)->text();
        QString checkNumber = ui->tw->item(row, 3)->text();
        reportText += " ID " + fineCode + " НАЗВАНИЕ " + amount + " САЙТ " + responsibility + ", связанная с факультетом №" + checkNumber + ";\n";
    }

    reportText += "\nС уважением,\n";
    reportText += "МГТУ им. Н. Э. Баумана";

    QString fileName = "groups_request.txt";

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << reportText;
        file.close();
        QMessageBox::information(this, "Успех", "Запрос на формирование отчета о Кафедрах успешно сохранен в файле " + fileName);


    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл для сохранения отчета о Кафедрах.");
    }
}


void onb2::on_tw_itemSelectionChanged()
{
    int curRow = ui->tw->currentRow();

    if (curRow < 0)
    {
        ui->cmb1->setCurrentIndex(0);
        ui->le1->clear();
        ui->le2->clear();
        ui->le3->clear();
        return;
    }

    QString pu1 = ui->tw->item(curRow, 0)->text(); // Изменил индекс столбца на 0, если это первый столбец с индексом 0
    QString pu2 = ui->tw->item(curRow, 1)->text(); // Изменил индекс столбца на 1
    QString pu3 = ui->tw->item(curRow, 2)->text(); // Изменил индекс столбца на 2

    int cmbIndex = ui->tw->item(curRow, 3)->text().toInt();

    ui->le1->setText(pu1);
    ui->le2->setText(pu2);
    ui->le3->setText(pu3);
    ui->cmb1->setCurrentIndex(cmbIndex);
}


void onb2::clean()
{
    ui->cmb1->setCurrentIndex(0);
    ui->le1->clear();
    ui->le2->clear();
    ui->le3->clear();
}

