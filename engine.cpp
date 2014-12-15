#include "engine.h"
#include "ui_engine.h"

Engine::Engine(const QString &hostName, const QString &user, const QString &pass, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Engine),
    hostName_(hostName),
    user_(user),
    pass_(pass),
    errorCount_(0),
    logLineCount(0),
    check(this)
{
    ui->setupUi(this);
    connect(ui->tables, SIGNAL(currentIndexChanged(QString)), this, SLOT(showMetaDb(QString)));
    connect(ui->databases, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeDatabase(QString)));
    connect(ui->update, SIGNAL(clicked()), this, SLOT(update()));
    connect(ui->check, SIGNAL(clicked()), this, SLOT(checkDatabase()));
    connect(ui->clear, SIGNAL(clicked()), this, SLOT(clearLog()));


    ui->progressBar->setValue(100);
    ui->plainTextEdit->setReadOnly(true);
    show();
    openConnection();
    model = new QSqlTableModel(this, meta_db);
    ui->tableView->setModel(model);

    if(setDatabaseName("meta_db"))
    {
        check.setText("Find database");
        check.setInformativeText("Сheck the relevance?");
        check.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        check.setDefaultButton(QMessageBox::Yes);
        switch(check.exec()){
            case QMessageBox::Yes:
                checkDatabase();
                break;
            case QMessageBox::No:
                break;
        }
    }
    else{
        createDatabase("meta_db");
    }
    QString str("SHOW databases;");
    QSqlQuery query;
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");
    while (query.next()) {
        QString db_name = query.value(0).toString();
        ui->databases->addItem(db_name);
    }

    str="SHOW tables;";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");
    while (query.next()) {
        QString tbl_name = query.value(0).toString();
        ui->tables->addItem(tbl_name);
    }
    ui->tableView->setModel(model);
}

Engine::~Engine()
{
    meta_db.close();
    delete ui;
}

void Engine::createDatabase(const QString &name)
{

    QString str("DROP DATABASE %1;");
    str = str.arg(name);
    QSqlQuery query;
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE DATABASE %1 \
        CHARACTER SET latin1 \
        COLLATE latin1_bin;";
    str = str.arg(name);
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="USE %1;";
    str = str.arg(name);
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE TABLE server (\
        server_id int(11) NOT NULL,\
        ip_port varchar(255) NOT NULL,\
        srv_user varchar(255) NOT NULL,\
        srv_pass varchar(255) NOT NULL,\
        PRIMARY KEY (server_id)\
        )\
            CHARACTER SET latin1\
            COLLATE latin1_bin;";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE TABLE `m_database` (\
        database_id int(11) NOT NULL,\
        server_id int(11) NOT NULL,\
        db_name varchar(255)  NOT NULL,\
        db_user varchar(255)  NOT NULL,\
        db_pass varchar(255)  NOT NULL,\
        PRIMARY KEY (database_id),\
        CONSTRAINT FK_database_server FOREIGN KEY (server_id)\
        REFERENCES server (server_id)\
        )\
            CHARACTER SET latin1\
            COLLATE latin1_bin";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE TABLE `m_table` (\
        table_id int(11) NOT NULL ,\
        database_id int(11) NOT NULL,\
        table_name varchar(255) NOT NULL,\
        PRIMARY KEY (table_id),\
        CONSTRAINT FK_table_database FOREIGN KEY (database_id)\
        REFERENCES `m_database` (database_id)\
        )\
            CHARACTER SET latin1\
            COLLATE latin1_bin;";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE TABLE attribute (\
        attribute_id int(11) NOT NULL,\
        attr_name varchar(255) NOT NULL,\
        attr_type varchar(255) NOT NULL,\
        table_id int(11) NOT NULL,\
        PRIMARY KEY (attribute_id),\
        CONSTRAINT FK_attribute_table FOREIGN KEY (table_id)\
        REFERENCES `m_table` (table_id)\
        )\
            CHARACTER SET latin1\
            COLLATE latin1_bin;";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE TABLE `m_key` (\
        key_id int(11) NOT NULL ,\
        table_id int(11) NOT NULL,\
        PRIMARY KEY (key_id),\
        CONSTRAINT FK_key_table FOREIGN KEY (table_id)\
        REFERENCES `m_table` (table_id)\
        )\
            CHARACTER SET latin1\
            COLLATE latin1_bin;";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE TABLE link (\
        link_id int(11) NOT NULL ,\
        key_PK int(11) NOT NULL,\
        key_FK int(11) NOT NULL,\
        PRIMARY KEY (link_id),\
        CONSTRAINT FK_link_key_FK FOREIGN KEY (key_FK)\
        REFERENCES `m_key` (key_id),\
        CONSTRAINT FK_link_key_PK FOREIGN KEY (key_PK)\
        REFERENCES `m_key` (key_id)\
        )\
            CHARACTER SET latin1\
            COLLATE latin1_bin;";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    str="CREATE TABLE content (\
            content_id int(11) NOT NULL,\
            key_id int(11) NOT NULL,\
            attr_id int(11) NOT NULL,\
            sequence int(11) NOT NULL,\
            PRIMARY KEY (content_id),\
            CONSTRAINT FK_content_attribute FOREIGN KEY (attr_id)\
            REFERENCES attribute (attribute_id),\
            CONSTRAINT FK_content_key FOREIGN KEY (key_id)\
            REFERENCES `m_key` (key_id)\
        )\
            CHARACTER SET latin1\
            COLLATE latin1_bin;";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");

    getDatabases();
}

void Engine::getDatabases()
{
    int db=1,tbl=1, attr=1, key=1, cnt=1, link=1;
    //================= server ==========================
    QString str("INSERT INTO `server` (server_id ,ip_port, srv_user, srv_pass) VALUES (1,\"%1\",\"%2\",\"%3\");");
    str = str.arg(hostName_).arg(user_).arg(pass_);
    QSqlQuery query;
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");
    //===================================================

    //================== database =======================
    query.exec("SHOW DATABASES;");
    while (query.next()) {
        QString name = query.value(0).toString();
        if((name == "meta_db" )|| (name == "temp_meta_db"))
            continue;
        QSqlQuery subquery;

        str = "INSERT INTO `m_database` (database_id, server_id, db_name, db_user, db_pass) VALUES (%4 ,1,\"%1\",\"%2\",\"%3\");";
        str = str.arg(name).arg(user_).arg(pass_).arg(db);
        if(!subquery.exec(str))
            setlogLine(subquery.lastError().databaseText()+"\n");

        //================== table =========================
        str = "SELECT table_name FROM information_schema.TABLES WHERE  TABLE_SCHEMA = '%1';";
        str = str.arg(name);
        if(!subquery.exec(str))
            setlogLine(subquery.lastError().databaseText()+"\n");
        while (subquery.next()) {
            QString tbl_name = subquery.value(0).toString();
            str = "INSERT INTO `m_table` (table_id, database_id, table_name) VALUES (%1,\"%2\",\"%3\");";
            str = str.arg(tbl).arg(db).arg(tbl_name);
            QSqlQuery subsubquery;
            if(!subsubquery.exec(str))
                setlogLine(subsubquery.lastError().databaseText()+"\n");

            //=============== attribute =========================
            str = "SELECT column_name, column_type FROM information_schema.columns WHERE table_name = '%1';";
            str = str.arg(tbl_name);
            if(!subsubquery.exec(str))
                setlogLine(subsubquery.lastError().databaseText()+"\n");
            while (subsubquery.next()) {
                QString attr_name = subsubquery.value(0).toString();
                QString attr_type = subsubquery.value(1).toString();

                str = "INSERT INTO attribute (attribute_id,  attr_name, attr_type, table_id) VALUES (%1,\"%2\",\"%3\", %4);";
                str = str.arg(attr).arg(attr_name).arg(attr_type).arg(tbl);
                QSqlQuery subsubsubquery;
                if(!subsubsubquery.exec(str))
                    setlogLine(subsubsubquery.lastError().databaseText()+"\n");
                attr++;

            }
            //===================================================

            //=============== Key ===============================
            str = "SELECT column_name, ordinal_position FROM information_schema.key_column_usage WHERE constraint_schema = '%1' AND TABLE_NAME = '%2';";
            str = str.arg(name).arg(tbl_name);
            if(!subsubquery.exec(str))
                setlogLine(subsubquery.lastError().databaseText()+"\n");
            while (subsubquery.next()) {
                QString attr_name = subsubquery.value(0).toString();
                int sequence = subsubquery.value(1).toInt();

                str = "INSERT INTO `m_key` (key_id, table_id) VALUES (%1, %2);";
                str = str.arg(key).arg(tbl);
                QSqlQuery subsubsubquery;
                if(!subsubsubquery.exec(str))
                    setlogLine(subsubsubquery.lastError().databaseText()+"\n");
                //============ Content =============================
                str = "SELECT attribute_id FROM attribute WHERE attr_name = '%1' AND table_id = '%2';";
                str = str.arg(attr_name).arg(tbl);
                if(!subsubsubquery.exec(str))
                    setlogLine(subsubsubquery.lastError().databaseText()+"\n");
                while (subsubsubquery.next()) {
                    int attr_id = subsubsubquery.value(0).toInt();
                    str = "INSERT INTO content (content_id, key_id, attr_id, `sequence`) VALUES (%1, %2, %3, %4);";
                    str = str.arg(cnt).arg(key).arg(attr_id).arg(sequence);
                    QSqlQuery sub4query;
                    if(!sub4query.exec(str))
                        setlogLine(sub4query.lastError().databaseText()+"\n");
                    else{
                        cnt++;
                    }


                }
                //==================================================
                key++;
            }
            //=====================================================
            tbl++;
        }
        //======================================================
        db++;
      }
    //======================================================

    //================= link ===============================
    str = "SELECT CONSTRAINT_SCHEMA,TABLE_NAME, column_name, REFERENCED_TABLE_SCHEMA, REFERENCED_TABLE_NAME, referenced_column_name \
           FROM information_schema.key_column_usage WHERE constraint_name != 'PRIMARY' AND referenced_column_name != '';";
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");
    while (query.next()) {
         QString c_db_name = query.value(0).toString();
         QString c_tbl_name = query.value(1).toString();
         QString c_attr_name = query.value(2).toString();
         QString r_db_name = query.value(3).toString();
         QString r_tbl_name = query.value(4).toString();
         QString r_attr_name = query.value(5).toString();
         if((c_db_name == "meta_db" )|| (c_db_name == "temp_meta_db"))
             continue;
         if((r_db_name == "meta_db" )|| (r_db_name == "temp_meta_db"))
             continue;

         str = "SELECT `m_key`.key_id, attr_name, `table_name`, db_name FROM `m_key`\
                 JOIN content ON `m_key`.key_id = content.key_id \
                 JOIN attribute ON content.attr_id = attribute.attribute_id\
                 JOIN `m_table` ON attribute.table_id = `m_table`.table_id \
                 JOIN `m_database` ON `m_table`.database_id = `m_database`.database_id \
                WHERE db_name = '%1' AND `table_name` = '%2' AND attr_name = '%3';";
         QString constr = str.arg(c_db_name).arg(c_tbl_name).arg(c_attr_name);

         QSqlQuery subquery;
         if(!subquery.exec(constr))
             setlogLine(subquery.lastError().databaseText()+"\n");
         subquery.first();
         int key_FK = subquery.value(0).toInt();

         QString ref = str.arg(r_db_name).arg(r_tbl_name).arg(r_attr_name);

         if(!subquery.exec(ref))
             setlogLine(subquery.lastError().databaseText()+"\n");
         subquery.first();
         int key_PK = subquery.value(0).toInt();

         str = "INSERT INTO `link` (link_id, key_FK, key_PK) VALUES (%1,%2,%3);";
         str = str.arg(link).arg(key_FK).arg(key_PK);
         if(!subquery.exec(str))
             setlogLine(subquery.lastError().databaseText()+"\n");
         link++;
    }
    //======================================================
}

void Engine::checkDatabase()
{
    errorCount_ = 0;
    ui->statusBar->text() = "checking...";
    ui->progressBar->setValue(0);
    createDatabase("temp_meta_db");
    ui->progressBar->setValue(10);
    if(!setDatabaseName("meta_db"))
        return;
    QStringList tables;
    QString str("SHOW TABLES;");
    QSqlQuery query;
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");
    while (query.next()) {
        tables.append(query.value(0).toString());
    }

    double step = 85/tables.size();
    ui->progressBar->setValue(13);
    foreach(QString tbl_name, tables)
    {
        str = "SHOW COLUMNS FROM %1;";
        str =str.arg(tbl_name);
        if(!query.exec(str))
            setlogLine(query.lastError().databaseText()+"\n");
        int columnCount = query.size();

        QString str1("SELECT * FROM meta_db.%1;");
        QString str2("SELECT * FROM temp_meta_db.%1;");

        str1 = str1.arg(tbl_name);
        str2 = str2.arg(tbl_name);
        QSqlQuery query_1;
        QSqlQuery query_2;
        if(!query_1.exec(str1))
            setlogLine(query_1.lastError().databaseText()+"\n");
        if(!query_2.exec(str2))
            setlogLine(query_2.lastError().databaseText()+"\n");
        int row = 0;
        while (query_1.next()) {
            if(!query_2.next()){
                setlogLine(QString("Meta database has error in table: %1 (row: %2)\n").arg(tbl_name).arg(row));
                errorCount_++;
                break;
            }
            bool br = false;
            for(int col = 0; col < columnCount; col++){
                QString str_1 = query_1.value(col).toString();
                QString str_2 = query_2.value(col).toString();
                if(str_1 != str_2){
                    setlogLine(QString("Meta database has error in table: %1 (row: %2, column: %3)\n").arg(tbl_name).arg(row).arg(col));
                    errorCount_++;
                    br = true;
                    break;
                }
            }
            if(br)
                break;
            row++;
        }
        if(query_2.next()){
            setlogLine(QString("Meta database has error in table: %1 (row: %2)\n").arg(tbl_name).arg(row));
            errorCount_++;
        }
        ui->progressBar->setValue(ui->progressBar->value()+step);
    }

    setDatabaseName("meta_db");
    str = ("DROP DATABASE temp_meta_db;");
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");
    ui->progressBar->setValue(100);
    ui->statusBar->text() = "";
    check.setText("Checking finished.");
    QString msg("Find %1 errors%2");

    msg = msg.arg(errorCount_);
    if(errorCount_){
        msg = msg.arg("(See logs).Update database?");
        check.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        check.setDefaultButton(QMessageBox::Yes);
        check.setInformativeText(msg);
    }else{
        msg = msg.arg(".");
        check.setStandardButtons(QMessageBox::Ok);
        check.setDefaultButton(QMessageBox::Ok);
        check.setInformativeText(msg);
    }
    switch(check.exec()){
        case QMessageBox::Yes:
            createDatabase("meta_db");
            break;
        default:
            break;

    }


}

bool Engine::setDatabaseName(const QString &name)
{
    QString str = "USE %1;";
    QSqlQuery query;
    if(!query.exec(str.arg(name)))
        return false;
    return true;
}


void Engine::showMetaDb(const QString &text)
{
    // указываем таблицу из БД
       model->setTable(text);
       // заносим данные в модель
       // если удачно
       if(model->select())
       {
          // передаем данные из модели в tableView
          ui->tableView->setModel(model);
          // устанавливаем высоту строки по тексту
          ui->tableView->resizeRowsToContents();
          // передача управления элементу tableView
          ui->tableView->setFocus();
       }else
       {
           setlogLine(model->lastError().text()+ "\n");
       }
       ui->tableView->setModel(model);
}

void Engine::changeDatabase(const QString &text)
{
    setDatabaseName(text);
    QString str="SHOW tables;";
    QSqlQuery query;
    if(!query.exec(str))
        setlogLine(query.lastError().databaseText()+"\n");
    ui->tables->clear();
    while (query.next()) {
        QString tbl_name = query.value(0).toString();
        ui->tables->addItem(tbl_name);
    }
}
