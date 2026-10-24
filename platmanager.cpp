#include "platmanager.h"
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

// DECIMAL(10,2) : 8 chiffres entiers + 2 décimales.
constexpr double PRIX_MAX = 99999999.99;

bool prixValide(double prix)
{
    return qIsFinite(prix) && prix >= 0.0 && prix <= PRIX_MAX;
}

bool platExiste(int idPlat)
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("SELECT 1 FROM plat WHERE id_plat = ?"));
    query.addBindValue(idPlat);
    return query.exec() && query.next();
}
}

PlatManager::PlatManager(QObject *parent)
    : QObject(parent)
{
}

QList<PlatData> PlatManager::getAll(const QString &search, QString *error) const
{
    QList<PlatData> result;

    QSqlQuery query(Database::instance()->getDatabase());
    QString sql = QStringLiteral("SELECT id_plat, nom_plat, prix FROM plat");
    const QString term = search.trimmed();
    if (!term.isEmpty()) {
        sql += QStringLiteral(" WHERE nom_plat LIKE ?");
    }
    sql += QStringLiteral(" ORDER BY nom_plat ASC");

    query.prepare(sql);
    if (!term.isEmpty()) {
        query.addBindValue(QStringLiteral("%") + term + QStringLiteral("%"));
    }

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger les plats : %1")
                            .arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        PlatData plat;
        plat.id = query.value(0).toInt();
        plat.nom = query.value(1).toString();
        plat.prix = query.value(2).toDouble();
        result.append(plat);
    }
    return result;
}

bool PlatManager::ajouter(const QString &nom, double prix, QString *error)
{
    const QString nomNettoye = nom.trimmed();
    if (nomNettoye.isEmpty()) {
        setError(error, QStringLiteral("Le nom du plat est obligatoire."));
        return false;
    }
    if (prix < 0.0) {
        setError(error, QStringLiteral("Le prix doit être positif ou nul."));
        return false;
    }
    if (!prixValide(prix)) {
        setError(error, QStringLiteral(
            "Le prix ne peut pas dépasser 99 999 999,99 Ar (limite de la base de données)."));
        return false;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("INSERT INTO plat (nom_plat, prix) VALUES (?, ?)"));
    query.addBindValue(nomNettoye);
    query.addBindValue(prix);

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible d'ajouter le plat : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool PlatManager::modifier(int idPlat, const QString &nom, double prix, QString *error)
{
    if (idPlat <= 0) {
        setError(error, QStringLiteral("Aucun plat sélectionné."));
        return false;
    }

    const QString nomNettoye = nom.trimmed();
    if (nomNettoye.isEmpty()) {
        setError(error, QStringLiteral("Le nom du plat est obligatoire."));
        return false;
    }
    if (prix < 0.0) {
        setError(error, QStringLiteral("Le prix doit être positif ou nul."));
        return false;
    }
    if (!prixValide(prix)) {
        setError(error, QStringLiteral(
            "Le prix ne peut pas dépasser 99 999 999,99 Ar (limite de la base de données)."));
        return false;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("UPDATE plat SET nom_plat = ?, prix = ? WHERE id_plat = ?"));
    query.addBindValue(nomNettoye);
    query.addBindValue(prix);
    query.addBindValue(idPlat);

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de modifier le plat : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    if (query.numRowsAffected() == 0) {
        // MySQL renvoie 0 ligne affectée si les valeurs sont inchangées.
        if (platExiste(idPlat)) {
            return true;
        }
        setError(error, QStringLiteral("Plat introuvable."));
        return false;
    }
    return true;
}

bool PlatManager::supprimer(int idPlat, QString *error)
{
    if (idPlat <= 0) {
        setError(error, QStringLiteral("Aucun plat sélectionné."));
        return false;
    }

    QSqlDatabase db = Database::instance()->getDatabase();
    if (!db.isOpen()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return false;
    }

    // Sauvegarder le nom du plat pour les snapshots des détails de commandes existants
    QString nomPlat;
    {
        QSqlQuery info(db);
        info.prepare(QStringLiteral("SELECT nom_plat FROM plat WHERE id_plat = ?"));
        info.addBindValue(idPlat);
        if (info.exec() && info.next()) {
            nomPlat = info.value(0).toString();
        }
    }

    // Mettre à jour les snapshots dans detail_commande via menu_jour lié
    if (!nomPlat.isEmpty()) {
        QSqlQuery snap(db);
        snap.prepare(QStringLiteral(
            "UPDATE detail_commande dc "
            "JOIN menu_jour m ON m.id_menu = dc.id_menu "
            "SET dc.nom_plat_snapshot = COALESCE(NULLIF(TRIM(dc.nom_plat_snapshot), ''), ?), "
            "    dc.nom_menu_snapshot = COALESCE(NULLIF(TRIM(dc.nom_menu_snapshot), ''), ?) "
            "WHERE m.id_plat = ?"));
        snap.addBindValue(nomPlat);
        snap.addBindValue(nomPlat);
        snap.addBindValue(idPlat);
        snap.exec();
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM plat WHERE id_plat = ?"));
    query.addBindValue(idPlat);

    if (!query.exec()) {
        const QSqlError err = query.lastError();
        if (err.nativeErrorCode() == QStringLiteral("1451")) {
            setError(error, QStringLiteral(
                "Suppression impossible : ce plat est toujours référencé.\n"
                "Les données historiques ont été conservées via les informations enregistrées."));
        } else {
            setError(error, QStringLiteral("Impossible de supprimer le plat : %1").arg(err.text()));
        }
        return false;
    }
    if (query.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Plat introuvable."));
        return false;
    }
    return true;
}

int PlatManager::compter(QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM plat")) || !query.next()) {
        setError(error, QStringLiteral("Impossible de compter les plats : %1")
                            .arg(query.lastError().text()));
        return 0;
    }
    return query.value(0).toInt();
}
