    #include "onb4.h"
#include "ui_onb4.h"
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
onb4::onb4(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::onb4)
{
    ui->setupUi(this);

    // Подключение сигналов к слотам
    connect(ui->btn1, SIGNAL(clicked(bool)), this, SLOT(add()));
    connect(ui->btn2, SIGNAL(clicked(bool)), this, SLOT(edit()));
    connect(ui->btn3, SIGNAL(clicked(bool)), this, SLOT(remove()));
    connect(ui->btn4, SIGNAL(clicked(bool)), this, SLOT(clean()));
    connect(ui->btnSearch, &QPushButton::clicked, this, &onb4::search);
    connect(ui->btnReport, &QPushButton::clicked, this, &onb4::generateReportRequest);
    AuthenticationManager authenticationManager;
    QString currentUser = authenticationManager.getCurrentUserLogin();
    if(currentUser == "DATAVIEWER")
    {
        ui->btn1->setVisible(false); // Скрыть кнопку 1
        ui->btn2->setVisible(false);
        ui->btn3->setVisible(false);
    }

    // Вызываем метод selectAll() через таймер с нулевой задержкой, чтобы он выполнился после инициализации интерфейса
    QTimer::singleShot(0, this, &onb4::selectAll);

    // Задаем количество столбцов в компоненте таблицы
    ui->tw->setColumnCount(4);

    // Задаем заголовки столбцов таблицы
    ui->tw->setHorizontalHeaderItem(0, new QTableWidgetItem("ID"));
    ui->tw->setHorizontalHeaderItem(1, new QTableWidgetItem("ФИО"));
    ui->tw->setHorizontalHeaderItem(2, new QTableWidgetItem("Номер Телефона"));
    ui->tw->setHorizontalHeaderItem(3, new QTableWidgetItem("Номер группы"));


    // Устанавливаем растягивание последнего столбца при изменении размера формы
    ui->tw->horizontalHeader()->setStretchLastSection(true);

    // Включаем возможность прокрутки содержимого таблицы
    ui->tw->setAutoScroll(true);

    // Устанавливаем режим выделения ячеек: только одна строка
    ui->tw->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tw->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Разрешаем пользователю сортировать данные по столбцам
    ui->tw->setSortingEnabled(true);
    ui->tw->sortByColumn(0, Qt::AscendingOrder);

    // Запрещаем редактирование ячеек таблицы
    ui->tw->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Заполнение выпадающих списков
    populate1ComboBox();

}

onb4::~onb4()
{
    delete ui;
}

void onb4::on_btnExit_clicked()
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



void onb4::populate1ComboBox()
{
    ui->cmb1->clear();
    QSqlQuery query("SELECT group_name FROM groups ORDER BY group_id");
    while (query.next()) {
        QString pu1 = query.value("group_name").toString();
        ui->cmb1->addItem( pu1);
    }
}
void onb4::selectAll()
{
    // Проверяем, открыто ли соединение с базой данных
    if (!DatabaseHelper::isDatabaseConnected())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена.");
        return;
    }


    QSqlQuery query(DatabaseHelper::getDatabaseConnection());


    QString sqlstr = "SELECT * from students";

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


void onb4::add()
{
    // Получаем введенные значения
    //QString name = ui->cmb1->currentText();
    //QString costStr = ui->le2->text();
    //QString periodNumberStr = ui->le3->text();
    QString periodNumberStr = ui->cmb1->currentText();
    QString name = ui->le2->text();
    QString costStr = ui->le3->text();
    // Проверяем, содержит ли имя цифры
    if (name.contains(QRegularExpression("\\d"))) {
        QMessageBox::critical(this, "Ошибка", "Имя не должно содержать цифр.");
        return;
    }
    // Подготавливаем запрос
    QSqlQuery query;
    query.prepare("INSERT INTO students (full_name, phone_number, group_id) "
                  "select :name, :cost, group_id from groups where group_name = :periodNumber");
    query.bindValue(":name", name);
    query.bindValue(":cost", costStr);
    query.bindValue(":periodNumber", periodNumberStr);

    // Выполняем запрос
    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        ui->teResult->append("Ошибка: " + query.lastError().text());
        return;
    }

    qDebug() << "Данные успешно добавлены в базу данных.";
    ui->teResult->append("Успех: данные успешно добавлены.");

    // Обновляем записи в компоненте таблицы
    selectAll();
}



void onb4::remove()
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
        QMessageBox::critical(this, "Ошибка", "Строка не выбрана!");
        return;
    }

    // Просим пользователя подтвердить удаление строки
    if (QMessageBox::question(this, "Удалить", "Удалить строку?", QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel)
        return;

    // Создаем объект запроса
    QSqlQuery query(DatabaseHelper::getDatabaseConnection());

    QString pu1 = ui->tw->item(curRow, 0)->text();

    // Подготавливаем строку запроса для удаления данных из таблицы
    QString sqlstr = "DELETE FROM students WHERE student_id = :organizationCode";

    // Подготавливаем запрос к выполнению
    query.prepare(sqlstr);
    query.bindValue(":organizationCode", pu1);

    // Выполняем запрос
    if (!query.exec())
    {
        // Проверяем текст ошибки на наличие фразы "foreign key"
        if (query.lastError().text().contains("foreign key", Qt::CaseInsensitive))
        {
            QMessageBox::critical(this, "Ошибка", "Невозможно удалить, так как есть связанные записи.");
        }
        else
        {
            // Если запрос не выполнен по другой причине, выводим сообщение об ошибке
            QMessageBox::critical(this, "Ошибка", query.lastError().text());
        }
        return;
    }

    // Выводим сообщение об удалении строки
    ui->teResult->append(QString("Удалено %1 строк").arg(query.numRowsAffected()));

    // Обновляем содержимое компонента таблицы
    selectAll();
}

void onb4::edit()
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

    // Проверяем, что имя не содержит цифр
    if (pu2.contains(QRegularExpression("\\d"))) {
        QMessageBox::critical(this, "Ошибка", "Имя не должно содержать цифр.");
        return;
    }
    // Получаем ID организации
    QString organizationCode = ui->tw->item(curRow, 0)->text();
    // Подготавливаем SQL запрос для обновления данных
    QSqlQuery query;
    query.prepare("UPDATE students SET full_name = :1, phone_number = :2, group_id  = (select group_id from groups where group_name = :3 "
                  "WHERE student_id = :4");
    query.bindValue(":1", pu2);
    query.bindValue(":2", pu3);
    query.bindValue(":3", cmb);
    query.bindValue(":4", organizationCode);

    // Выполняем запрос
    if (!query.exec()) {
        ui->teResult->append("Ошибка: " + query.lastError().text());
        return;
    }

    ui->teResult->append("Успех: данные успешно обновлены.");
    selectAll(); // Обновить таблицу после обновления
}


void onb4::search()
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
void onb4::generateReportRequest()
{
    // Вызываем функцию selectAll для получения данных о поставщиках из таблицы
    selectAll();

    // Создаем заголовок отчета
    QString reportText = "Отчет о студентах:\n\n";
    reportText += "Уважаемые коллеги,\n\n";
    reportText += "Подготовлен отчет о студентах:\n";

    // Добавляем информацию о каждом поставщике из таблицы
    for (int row = 0; row < ui->tw->rowCount(); ++row) {
        QString organizationCode = ui->tw->item(row, 0)->text();
        QString name = ui->tw->item(row, 1)->text();
        QString cost = ui->tw->item(row, 2)->text();
        QString periodNumber = ui->tw->item(row, 3)->text();
        reportText += "- Студент с кодом " + organizationCode + ", именем " + name + ", с номером телефона " + cost + ", из группы " + periodNumber + ";\n";
    }

    reportText += "\nС уважением,\n";
    reportText += "МГТУ им. Н. Э. Баумана"; // Замените "Ваше имя" на ваше имя или название компании

    // Имя файла для сохранения отчета
    QString fileName = "students_report.txt";

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << reportText;
        file.close();
        QMessageBox::information(this, "Успех", "Отчет о студентах успешно сохранен в файле " + fileName);
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл для сохранения отчета о студентах.");
    }
}


void onb4::on_tw_itemSelectionChanged()
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
    QString cmb = ui->tw->item(curRow, 3)->text(); // Изменил индекс столбца на 3

    ui->le1->setText(pu1);
    ui->le2->setText(pu2);
    ui->le3->setText(pu3);
    ui->cmb1->setCurrentText(cmb);
}


void onb4::clean()
{
    ui->cmb1->setCurrentIndex(0);
    ui->le1->clear();
    ui->le2->clear();
    ui->le3->clear();
}

