#include "onb6.h"
#include "ui_onb6.h"
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
onb6::onb6(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::onb6)
{
    ui->setupUi(this);

    // Подключение сигналов к слотам
    connect(ui->btn1, SIGNAL(clicked(bool)), this, SLOT(add()));
    connect(ui->btn2, SIGNAL(clicked(bool)), this, SLOT(edit()));
    connect(ui->btn3, SIGNAL(clicked(bool)), this, SLOT(remove()));
    connect(ui->btn4, SIGNAL(clicked(bool)), this, SLOT(clean()));
    connect(ui->btnSearch, &QPushButton::clicked, this, &onb6::search);
    connect(ui->btnReport, &QPushButton::clicked, this, &onb6::generateReportRequest);
    AuthenticationManager authenticationManager;
    QString currentUser = authenticationManager.getCurrentUserLogin();
    if(currentUser == "DATAVIEWER")
    {
        ui->btn1->setVisible(false); // Скрыть кнопку 1
        ui->btn2->setVisible(false);
        ui->btn3->setVisible(false);
    }
    // Вызываем метод selectAll() через таймер с нулевой задержкой, чтобы он выполнился после инициализации интерфейса
    QTimer::singleShot(0, this, &onb6::selectAll);

    // Задаем количество столбцов в компоненте таблицы
    ui->tw->setColumnCount(5);

    // Задаем заголовки столбцов таблицы
    ui->tw->setHorizontalHeaderItem(0, new QTableWidgetItem("ID"));
    ui->tw->setHorizontalHeaderItem(1, new QTableWidgetItem("ФИО"));
    ui->tw->setHorizontalHeaderItem(2, new QTableWidgetItem("Зарплата"));
    ui->tw->setHorizontalHeaderItem(3, new QTableWidgetItem("Номер телефона"));
    ui->tw->setHorizontalHeaderItem(4, new QTableWidgetItem("Кафедра"));


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

onb6::~onb6()
{
    delete ui;
}


void onb6::on_btnExit_clicked()
{
    // Создаем объект AuthenticationManager
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



void onb6::populate1ComboBox()
{
    ui->cmb1->clear();
    QSqlQuery query("SELECT id, name FROM departments ORDER BY id");
    while (query.next()) {
        QString pu1 = query.value("id").toString();
        QString pu2 = query.value("name").toString();
        ui->cmb1->addItem(pu2, pu1);
    }
}

void onb6::selectAll()
{
    // Проверяем, открыто ли соединение с базой данных
    if (!DatabaseHelper::isDatabaseConnected())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена.");
        return;
    }

    // Создаем объект запроса с привязкой к установленному соединению с базой данных
    QSqlQuery query(DatabaseHelper::getDatabaseConnection());

    // Создаем строку запроса на выборку всех данных из таблицы services без сортировки
    QString sqlstr = "SELECT * FROM teachers ORDER BY id";

    if (!query.exec(sqlstr))
    {
        QMessageBox::critical(this, "Ошибка", query.lastError().text());
        return;
    }

    // Очищаем таблицу перед обновлением
    ui->tw->clearContents();

    // Подгоняем количество строк таблицы под количество записей в результате запроса
    ui->tw->setRowCount(0); // Очищаем все строки

    int rowCount = 0;

    // Прочитать в цикле все строки результата (курсора)
    while (query.next())
    {
        // Добавляем новую строку в таблицу
        ui->tw->insertRow(rowCount);

        // Устанавливаем идентификатор строки
        QTableWidgetItem *idItem = new QTableWidgetItem(query.value("id").toString());
        ui->tw->setItem(rowCount, 0, idItem);
        // Заполняем значениями из базы данных
        ui->tw->setItem(rowCount, 1, new QTableWidgetItem(query.value("full_name").toString()));
        ui->tw->setItem(rowCount, 2, new QTableWidgetItem(query.value("salary").toString()));
        ui->tw->setItem(rowCount, 3, new QTableWidgetItem(query.value("phone_number").toString()));
        ui->tw->setItem(rowCount, 4, new QTableWidgetItem(query.value("department_id").toString()));
        // Устанавливаем выравнивание содержимого ячеек по центру
        for (int j = 0; j < ui->tw->columnCount(); ++j)
        {
            ui->tw->item(rowCount, j)->setTextAlignment(Qt::AlignCenter);
        }
        rowCount++;
    }
    // Подгоняем размер столбцов под содержимое
    ui->tw->resizeColumnsToContents();
}


void onb6::add()
{
    QString cmb1 = ui->cmb1->currentText();
    QString pu1 = ui->le2->text();  // Получаем введенное имя
    QString pu2 = ui->le3->text(); // Получаем введенное зп
    QString pu3 = ui->le4->text(); // Получаем введенный телефон

    // Проверяем, содержит ли имя цифры
    if (pu1.contains(QRegularExpression("\\d"))) {
        QMessageBox::critical(this, "Ошибка", "Имя не должно содержать цифр.");
        return;
    }
    // Подготавливаем SQL запрос для вставки данных
    QSqlQuery query;
    query.prepare("INSERT INTO teachers (full_name, salary, phone_number, department_id) "
                  "select :1, :2, :3, id from departments where name = :4");

    // Привязываем значения к параметрам запроса
    query.bindValue(":1", pu1);
    query.bindValue(":2", pu2);
    query.bindValue(":3", pu3);
    query.bindValue(":4", cmb1);

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


void onb6::remove()
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

    query.prepare("DELETE FROM teachers WHERE teacher_id = :pupu");
    query.bindValue(":pupu", pu1);

    // Выполняем запрос
    if (!query.exec()) {
        // Проверяем текст ошибки на наличие фразы "foreign key"
        if (query.lastError().text().contains("foreign key", Qt::CaseInsensitive))
        {
            QMessageBox::critical(this, "Ошибка", "Невозможно удалить, так как есть связанные.");
        }
        else
        {
            // Если запрос не выполнен по другой причине, выводим сообщение об ошибке
            QMessageBox::critical(this, "Ошибка", query.lastError().text());
        }
        return;
    }

    // Выводим сообщение об удалении строки
    ui->teResult->append(QString("Deleted %1 rows").arg(query.numRowsAffected()));

    // Обновляем содержимое компонента таблицы
    selectAll();
}

void onb6::edit()
{
    // Получить выбранные значения из выпадающих списков
    QString cmb = ui->cmb1->currentText(); // Получаем текст из первого выпадающего списка
    QString pu1 = ui->le1->text();
    QString pu2 = ui->le2->text();
    QString pu3 = ui->le3->text();
    QString pu4 = ui->le4->text();
    // Получаем текущую выбранную строку
    int curRow = ui->tw->currentRow();
    // Проверяем, что строка действительно выбрана
    if (curRow < 0)
    {
        ui->teResult->append("Внимание: строка не выбрана!");
        return;
    }
    QString consumerCode = ui->tw->item(curRow, 0)->text();
    // Проверяем, что имя не содержит цифр
    if (pu2.contains(QRegularExpression("\\d"))) {
        QMessageBox::critical(this, "Ошибка", "Имя не должно содержать цифр.");
        return;
    }
    // Подготавливаем SQL запрос для обновления данных
    QSqlQuery query;
    query.prepare("UPDATE teachers SET full_name = :Name, salary = :Debt, phone_number = :Site_Number, department_id = (select id from departments where name = :code) "
                  "WHERE id = :id");
    query.bindValue(":Name", pu2);
    query.bindValue(":Debt", pu3);
    query.bindValue(":Site_Number", pu4);
    query.bindValue(":code", cmb);
    query.bindValue(":id", consumerCode);

    // Выполняем запрос
    if (!query.exec()) {
        ui->teResult->append("Ошибка: " + query.lastError().text());
        return;
    }

    ui->teResult->append("Успех: данные успешно обновлены.");
    selectAll(); // Обновить таблицу после обновления
}

void onb6::search()
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
void onb6::generateReportRequest()
{
    // Вызываем функцию selectAll для получения данных о потребителях из таблицы
    selectAll();

    QString reportText = "Запрос на формирование отчета о Преподавателях:\n\n";
    reportText += "Уважаемые сотрудники,\n\n";
    reportText += "Подготовлен отчет о преподавателях:\n";

    // Добавляем информацию о каждом потребителе из таблицы
    for (int row = 0; row < ui->tw->rowCount(); ++row) {
        QString consumerCode = ui->tw->item(row, 0)->text();
        QString consumerName = ui->tw->item(row, 1)->text();
        QString debt = ui->tw->item(row, 2)->text();
        QString siteName = ui->tw->item(row, 3)->text();
        QString supplierName = ui->tw->item(row, 4)->text();
        reportText += "- Преподватель " + consumerName + " с кодом " + consumerCode + " имеет зарплату" + debt + " имеет телефон " + siteName + ", работает на кафедре " + supplierName + ";\n";
    }

    reportText += "\nС уважением,\n";
    reportText += "МГТУ им. Н. Э. Баумана";

    QString fileName = "teachers.txt"; // Имя файла для сохранения отчета

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << reportText;
        file.close();
        QMessageBox::information(this, "Успех", "Запрос на формирование отчета об учителях успешно сохранен в файле " + fileName);
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл для сохранения отчета об учителях.");
    }
}


void onb6::on_tw_itemSelectionChanged()
{
    int curRow = ui->tw->currentRow();

    if (curRow < 0)
    {
        ui->cmb1->setCurrentIndex(0);
        ui->le1->clear();
        ui->le2->clear();
        ui->le3->clear();
        ui->le4->clear();
        return;
    }

    QString pu1 = ui->tw->item(curRow, 0)->text(); // Изменил индекс столбца на 0, если это первый столбец с индексом 0
    QString pu2 = ui->tw->item(curRow, 1)->text(); // Изменил индекс столбца на 1
    QString pu3 = ui->tw->item(curRow, 2)->text(); // Изменил индекс столбца на 2
    QString pu4 = ui->tw->item(curRow, 3)->text(); // Изменил индекс столбца на 3
    int cmbIndex = ui->tw->item(curRow, 4)->text().toInt();
    ui->le1->setText(pu1);
    ui->le2->setText(pu2);
    ui->le3->setText(pu3);
    ui->le4->setText(pu4);
    ui->cmb1->setCurrentIndex(cmbIndex);
}


void onb6::clean()
{
    ui->cmb1->setCurrentIndex(0);
    ui->le1->clear();
    ui->le2->clear();
    ui->le3->clear();
    ui->le4->clear();
}

