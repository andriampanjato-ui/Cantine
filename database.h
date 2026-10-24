#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

class Database : public QObject
{
    Q_OBJECT
public:
    static Database* instance();
    
    bool connectToDatabase(const QString &host = "127.0.0.1",
                          int port = 3306,
                          const QString &dbName = "cantine_universitaire",
                          const QString &user = "root",
                          const QString &password = "");
    
    bool isConnected() const;
    void close();
    
    QSqlDatabase getDatabase() const;
    QString lastError() const;
    
    static bool isMysqlDriverAvailable();
    static QStringList availableDrivers();

    // Teste des paramètres sans toucher à la connexion active.
    static bool testerParametres(const QString &host, int port, const QString &dbName,
                                 const QString &user, const QString &password,
                                 QString *error = nullptr);

private:
    explicit Database(QObject *parent = nullptr);
    static Database *m_instance;
    
    QSqlDatabase m_db;
    QString m_lastError;
};

#endif // DATABASE_H
