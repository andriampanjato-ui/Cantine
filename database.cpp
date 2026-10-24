#include "database.h"
#include <QSqlError>
#include <QDebug>

Database* Database::m_instance = nullptr;

Database::Database(QObject *parent)
    : QObject(parent)
{
}

Database* Database::instance()
{
    if (m_instance == nullptr) {
        m_instance = new Database();
    }
    return m_instance;
}

bool Database::isMysqlDriverAvailable()
{
    return QSqlDatabase::isDriverAvailable("QMYSQL");
}

QStringList Database::availableDrivers()
{
    return QSqlDatabase::drivers();
}

bool Database::testerParametres(const QString &host, int port, const QString &dbName,
                                const QString &user, const QString &password,
                                QString *error)
{
    if (!isMysqlDriverAvailable()) {
        if (error) {
            *error = QStringLiteral("Le pilote QMYSQL n'est pas disponible.");
        }
        return false;
    }

    static int compteur = 0;
    const QString connectionName = QStringLiteral("cantine_test_%1").arg(++compteur);

    bool ok = false;
    QString message;
    {
        QSqlDatabase test = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connectionName);
        test.setHostName(host);
        test.setPort(port);
        test.setDatabaseName(dbName);
        test.setUserName(user);
        test.setPassword(password);

        ok = test.open();
        if (!ok) {
            message = test.lastError().text();
        }
        test.close();
    }
    QSqlDatabase::removeDatabase(connectionName);

    if (!ok && error) {
        *error = message;
    }
    return ok;
}

bool Database::connectToDatabase(const QString &host, int port, const QString &dbName,
                                const QString &user, const QString &password)
{
    m_lastError.clear();

    if (!isMysqlDriverAvailable()) {
        m_lastError = QStringLiteral(
            "Le pilote QMYSQL n'est pas disponible.\n\n"
            "Vérifiez que le pilote MySQL de Qt est installé.\n"
            "Si nécessaire, compilez le plugin QMYSQL avec votre installation Qt.\n\n"
            "Pilotes actuellement disponibles : %1")
            .arg(availableDrivers().join(", "));
        return false;
    }

    const QString connectionName = QStringLiteral("cantine_connection");

    // Réutiliser la connexion existante pour éviter les doublons de connexion.
    if (QSqlDatabase::contains(connectionName)) {
        m_db = QSqlDatabase::database(connectionName, false);
        if (m_db.isOpen()) {
            m_db.close();
        }
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connectionName);
    }

    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (!m_db.open()) {
        QSqlError err = m_db.lastError();
        m_lastError = QStringLiteral(
            "Échec de connexion à MySQL.\n\n"
            "Erreur : %1\n\n"
            "Vérifiez :\n"
            "- que XAMPP / MySQL est démarré (port 3306) ;\n"
            "- que la base '%2' existe (exécutez sql/cantine_universitaire.sql) ;\n"
            "- les paramètres de connexion (hôte, port, utilisateur, mot de passe).")
            .arg(err.text(), dbName);
        return false;
    }

    return true;
}

bool Database::isConnected() const
{
    return m_db.isOpen();
}

void Database::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

QSqlDatabase Database::getDatabase() const
{
    return m_db;
}

QString Database::lastError() const
{
    return m_lastError;
}
