#ifndef ENGINE_H
#define ENGINE_H

#include <QWidget>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QErrorMessage>
#include <ui_engine.h>

namespace Ui {
class Engine;
}

class Engine : public QWidget
{
    Q_OBJECT

public:
    explicit Engine(const QString &hostName, const QString &user, const QString &pass, QWidget *parent = 0);
    ~Engine();

    void getDatabases();
    bool setDatabaseName(const QString &name);
    void createDatabase(const QString &name);
signals:

    void logLineChanged(QString arg);

public slots:
    void clearLog(){
        m_logLine = "------ clear -----\n";
        logLineCount = 1;
        ui->plainTextEdit->setPlainText(m_logLine);
    }
    void update(){
        ui->progressBar->setValue(0);
        createDatabase("meta_db");
        ui->progressBar->setValue(100);
    }
    void setlogLine(QString arg)
    {
        if(logLineCount < 150){
            m_logLine = m_logLine + arg;
            logLineCount++;
        }else{
            m_logLine.remove(0, m_logLine.indexOf("\n"));
            m_logLine = m_logLine + arg;
        }

        ui->plainTextEdit->setPlainText(m_logLine);
    }
    void showMetaDb(const QString &text);
    void openConnection(){
        meta_db = QSqlDatabase::addDatabase("QMYSQL");
        meta_db.setHostName(hostName_);
        meta_db.setUserName(user_);
        meta_db.setPassword(pass_);
        if(!meta_db.open())
        {
            setlogLine("Database meta_db could'n open: " + meta_db.lastError().databaseText() + "\n");
            return;
        }
        setlogLine("Database meta_db open\n");
    }
    void changeDatabase(const QString &text);
    void checkDatabase();

private:
    Ui::Engine *ui;
    int errorCount_;
    int logLineCount;
    QString m_logLine;
    QSqlDatabase meta_db;
    QSqlTableModel *model;
    QString hostName_;
    QString user_;
    QString pass_;
//    int  db,tbl, attr, key, cnt, link;
    QProgressDialog *pd;
    QMessageBox check;

};

#endif // ENGINE_H
