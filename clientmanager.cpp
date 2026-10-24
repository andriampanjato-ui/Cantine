#include "clientmanager.h"
#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace {
void setError(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
}

bool clientExiste(int idClient)
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("SELECT 1 FROM client WHERE id_client = ?"));
    query.addBindValue(idClient);
    return query.exec() && query.next();
}
}

ClientManager::ClientManager(QObject *parent)
    : QObject(parent)
{
}

QList<ClientData> ClientManager::getAll(const QString &search, QString *error) const
{
    QList<ClientData> result;

    if (!Database::instance()->isConnected()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return result;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    QString sql = QStringLiteral("SELECT id_client, nom, telephone FROM client");
    const QString term = search.trimmed();
    if (!term.isEmpty()) {
        sql += QStringLiteral(" WHERE nom LIKE ? OR telephone LIKE ?");
    }
    sql += QStringLiteral(" ORDER BY nom ASC, id_client ASC");

    query.prepare(sql);
    if (!term.isEmpty()) {
        const QString pattern = QStringLiteral("%") + term + QStringLiteral("%");
        query.addBindValue(pattern);
        query.addBindValue(pattern);
    }

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger les clients : %1")
                            .arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        ClientData client;
        client.id = query.value(0).toInt();
        client.nom = query.value(1).toString();
        client.telephone = query.value(2).toString();
        result.append(client);
    }
    return result;
}

bool ClientManager::ajouter(const QString &nom, const QString &telephone, QString *error)
{
    const QString nomNettoye = nom.trimmed();
    if (nomNettoye.isEmpty()) {
        setError(error, QStringLiteral("Le nom du client est obligatoire."));
        return false;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("INSERT INTO client (nom, telephone) VALUES (?, ?)"));
    query.addBindValue(nomNettoye);
    query.addBindValue(telephone.trimmed());

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible d'ajouter le client : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool ClientManager::modifier(int idClient, const QString &nom, const QString &telephone, QString *error)
{
    if (idClient <= 0) {
        setError(error, QStringLiteral("Aucun client sélectionné."));
        return false;
    }

    const QString nomNettoye = nom.trimmed();
    if (nomNettoye.isEmpty()) {
        setError(error, QStringLiteral("Le nom du client est obligatoire."));
        return false;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("UPDATE client SET nom = ?, telephone = ? WHERE id_client = ?"));
    query.addBindValue(nomNettoye);
    query.addBindValue(telephone.trimmed());
    query.addBindValue(idClient);

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de modifier le client : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    if (query.numRowsAffected() == 0) {
        // MySQL renvoie 0 ligne affectée lorsque les valeurs sont identiques :
        // ce n'est pas une erreur si le client existe toujours.
        if (clientExiste(idClient)) {
            return true;
        }
        setError(error, QStringLiteral("Client introuvable."));
        return false;
    }
    return true;
}

bool ClientManager::supprimer(int idClient, QString *error)
{
    if (idClient <= 0) {
        setError(error, QStringLiteral("Aucun client sélectionné."));
        return false;
    }

    QSqlDatabase db = Database::instance()->getDatabase();
    if (!db.isOpen()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return false;
    }

    // Sauvegarder les informations du client avant suppression pour les snapshots.
    QString nomClient;
    QString telephoneClient;
    {
        QSqlQuery info(db);
        info.prepare(QStringLiteral("SELECT nom, telephone FROM client WHERE id_client = ?"));
        info.addBindValue(idClient);
        if (info.exec() && info.next()) {
            nomClient = info.value(0).toString();
            telephoneClient = info.value(1).toString();
        }
    }

    // Mettre à jour les snapshots des commandes existantes avec les données du client.
    {
        QSqlQuery snap(db);
        snap.prepare(QStringLiteral(
            "UPDATE commande SET "
            "nom_client_snapshot = COALESCE(NULLIF(TRIM(nom_client_snapshot), ''), ?), "
            "telephone_client_snapshot = COALESCE(NULLIF(TRIM(telephone_client_snapshot), ''), ?) "
            "WHERE id_client = ?"));
        snap.addBindValue(nomClient);
        snap.addBindValue(telephoneClient);
        snap.addBindValue(idClient);
        if (!snap.exec()) {
            setError(error, QStringLiteral("Impossible de sauvegarder l'historique du client : %1")
                                .arg(snap.lastError().text()));
            return false;
        }
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM client WHERE id_client = ?"));
    query.addBindValue(idClient);

    if (!query.exec()) {
        const QSqlError err = query.lastError();
        if (err.nativeErrorCode() == QStringLiteral("1451")) {
            setError(error, QStringLiteral(
                "Suppression impossible : ce client est toujours référencé.\n"
                "Les données historiques doivent être préservées."));
        } else {
            setError(error, QStringLiteral("Impossible de supprimer le client : %1").arg(err.text()));
        }
        return false;
    }
    if (query.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Client introuvable."));
        return false;
    }
    return true;
}

int ClientManager::compter(QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM client")) || !query.next()) {
        setError(error, QStringLiteral("Impossible de compter les clients : %1")
                            .arg(query.lastError().text()));
        return 0;
    }
    return query.value(0).toInt();
}

QString ClientManager::nomClient(int idClient, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("SELECT nom FROM client WHERE id_client = ?"));
    query.addBindValue(idClient);
    if (!query.exec() || !query.next()) {
        setError(error, QStringLiteral("Client introuvable."));
        return QString();
    }
    return query.value(0).toString();
}
