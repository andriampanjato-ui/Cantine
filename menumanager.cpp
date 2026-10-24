#include "menumanager.h"
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
}

MenuManager::MenuManager(QObject *parent)
    : QObject(parent)
{
}

QList<MenuData> MenuManager::chargerMenus(const QString &whereClause,
                                         const QList<QVariant> &params,
                                         const QString &havingClause,
                                         const QString &orderClause,
                                         QString *error) const
{
    QList<MenuData> result;

    QString sql = QStringLiteral(
        "SELECT m.id_menu, m.id_plat, p.nom_plat, p.prix, m.date_menu, m.quantite_initiale, "
        "       COALESCE(SUM(CASE WHEN c.statut = 'VALIDEE' THEN dc.quantite ELSE 0 END), 0) AS qte_cmd, "
        "       (m.quantite_initiale - COALESCE(SUM(CASE WHEN c.statut = 'VALIDEE' THEN dc.quantite ELSE 0 END), 0)) AS qte_rest "
        "FROM menu_jour m "
        "INNER JOIN plat p ON p.id_plat = m.id_plat "
        "LEFT JOIN detail_commande dc ON dc.id_menu = m.id_menu "
        "LEFT JOIN commande c ON c.id_commande = dc.id_commande ");

    if (!whereClause.isEmpty()) {
        sql += QStringLiteral("WHERE ") + whereClause + QStringLiteral(" ");
    }
    sql += QStringLiteral(
        "GROUP BY m.id_menu, m.id_plat, p.nom_plat, p.prix, m.date_menu, m.quantite_initiale ");
    if (!havingClause.isEmpty()) {
        sql += QStringLiteral("HAVING ") + havingClause + QStringLiteral(" ");
    }
    if (!orderClause.isEmpty()) {
        sql += QStringLiteral("ORDER BY ") + orderClause;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(sql);
    for (const QVariant &value : params) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger les menus : %1")
                            .arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        MenuData menu;
        menu.idMenu = query.value(0).toInt();
        menu.idPlat = query.value(1).toInt();
        menu.nomPlat = query.value(2).toString();
        menu.prix = query.value(3).toDouble();
        menu.dateMenu = query.value(4).toDate();
        menu.quantiteInitiale = query.value(5).toInt();
        menu.quantiteCommandee = query.value(6).toInt();
        menu.quantiteRestante = query.value(7).toInt();
        result.append(menu);
    }
    return result;
}

QList<MenuData> MenuManager::getMenusParDate(const QDate &date, QString *error) const
{
    return chargerMenus(QStringLiteral("m.date_menu = ?"),
                        {date.toString(QStringLiteral("yyyy-MM-dd"))},
                        QString(),
                        QStringLiteral("p.nom_plat ASC"),
                        error);
}

QList<MenuData> MenuManager::getMenusDisponibles(const QDate &date, QString *error) const
{
    return chargerMenus(QStringLiteral("m.date_menu = ?"),
                        {date.toString(QStringLiteral("yyyy-MM-dd"))},
                        QStringLiteral("qte_rest > 0"),
                        QStringLiteral("p.nom_plat ASC"),
                        error);
}

QList<MenuData> MenuManager::getMenusStockFaible(int seuil, QString *error) const
{
    return chargerMenus(QStringLiteral("m.date_menu = CURDATE()"),
                        {},
                        QStringLiteral("qte_rest <= %1 AND m.quantite_initiale > 0").arg(seuil),
                        QStringLiteral("qte_rest ASC, p.nom_plat ASC LIMIT 10"),
                        error);
}

bool MenuManager::ajouter(int idPlat, const QDate &dateMenu, int quantiteInitiale, QString *error)
{
    if (idPlat <= 0) {
        setError(error, QStringLiteral("Sélectionnez un plat."));
        return false;
    }
    if (!dateMenu.isValid()) {
        setError(error, QStringLiteral("La date du menu est invalide."));
        return false;
    }
    if (quantiteInitiale < 0) {
        setError(error, QStringLiteral("La quantité initiale doit être positive ou nulle."));
        return false;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "INSERT INTO menu_jour (id_plat, date_menu, quantite_initiale) VALUES (?, ?, ?)"));
    query.addBindValue(idPlat);
    query.addBindValue(dateMenu.toString(QStringLiteral("yyyy-MM-dd")));
    query.addBindValue(quantiteInitiale);

    if (!query.exec()) {
        const QSqlError err = query.lastError();
        if (err.nativeErrorCode() == QStringLiteral("1062")) {
            setError(error, QStringLiteral(
                "Ce plat est déjà présent dans le menu de cette date.\n"
                "Un même plat ne peut figurer qu'une seule fois par jour."));
        } else {
            setError(error, QStringLiteral("Impossible d'ajouter le menu : %1").arg(err.text()));
        }
        return false;
    }
    return true;
}

bool MenuManager::modifier(int idMenu, int quantiteInitiale, QString *error)
{
    if (idMenu <= 0) {
        setError(error, QStringLiteral("Aucun menu sélectionné."));
        return false;
    }
    if (quantiteInitiale < 0) {
        setError(error, QStringLiteral("La quantité initiale doit être positive ou nulle."));
        return false;
    }

    QSqlDatabase db = Database::instance()->getDatabase();

    // Transaction + verrou de ligne : une validation de commande concurrente
    // attend la fin de cette mise à jour avant de consommer du stock, ce qui
    // évite de fixer une quantité initiale inférieure au déjà commandé.
    QSqlQuery transaction(db);
    if (!transaction.exec(QStringLiteral("SET TRANSACTION ISOLATION LEVEL READ COMMITTED"))
        || !transaction.exec(QStringLiteral("START TRANSACTION"))) {
        setError(error, QStringLiteral("Impossible de démarrer la transaction : %1")
                            .arg(transaction.lastError().text()));
        return false;
    }

    const auto rollback = [&db]() {
        QSqlQuery query(db);
        query.exec(QStringLiteral("ROLLBACK"));
    };

    {
        QSqlQuery lock(db);
        lock.prepare(QStringLiteral(
            "SELECT quantite_initiale FROM menu_jour WHERE id_menu = ? FOR UPDATE"));
        lock.addBindValue(idMenu);
        if (!lock.exec()) {
            rollback();
            setError(error, QStringLiteral("Erreur SQL lors du verrouillage du menu : %1")
                                .arg(lock.lastError().text()));
            return false;
        }
        if (!lock.next()) {
            rollback();
            setError(error, QStringLiteral("Menu introuvable."));
            return false;
        }
    }

    int dejaCommandee = 0;
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "SELECT COALESCE(SUM(dc.quantite), 0) "
            "FROM detail_commande dc "
            "INNER JOIN commande c ON c.id_commande = dc.id_commande "
            "WHERE dc.id_menu = ? AND c.statut = 'VALIDEE'"));
        query.addBindValue(idMenu);
        if (!query.exec() || !query.next()) {
            rollback();
            setError(error, QStringLiteral("Impossible de vérifier les quantités déjà commandées."));
            return false;
        }
        dejaCommandee = query.value(0).toInt();
    }

    if (quantiteInitiale < dejaCommandee) {
        rollback();
        setError(error, QStringLiteral(
            "La quantité initiale (%1) ne peut pas être inférieure à la quantité déjà commandée (%2).")
            .arg(quantiteInitiale).arg(dejaCommandee));
        return false;
    }

    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("UPDATE menu_jour SET quantite_initiale = ? WHERE id_menu = ?"));
        query.addBindValue(quantiteInitiale);
        query.addBindValue(idMenu);
        if (!query.exec()) {
            rollback();
            setError(error, QStringLiteral("Impossible de modifier le menu : %1")
                                .arg(query.lastError().text()));
            return false;
        }
        // numRowsAffected() vaut 0 si la valeur est inchangée : le menu a été
        // trouvé et verrouillé plus haut, ce n'est donc pas une erreur.
    }

    if (!transaction.exec(QStringLiteral("COMMIT"))) {
        rollback();
        setError(error, QStringLiteral("Impossible de valider la modification : %1")
                            .arg(transaction.lastError().text()));
        return false;
    }
    return true;
}

bool MenuManager::supprimer(int idMenu, QString *error)
{
    if (idMenu <= 0) {
        setError(error, QStringLiteral("Aucun menu sélectionné."));
        return false;
    }

    QSqlDatabase db = Database::instance()->getDatabase();
    if (!db.isOpen()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return false;
    }

    QString nomPlat;
    {
        QSqlQuery info(db);
        info.prepare(QStringLiteral(
            "SELECT p.nom_plat FROM menu_jour m LEFT JOIN plat p ON p.id_plat = m.id_plat WHERE m.id_menu = ?"));
        info.addBindValue(idMenu);
        if (info.exec() && info.next()) {
            nomPlat = info.value(0).toString();
        }
    }

    if (!nomPlat.isEmpty()) {
        QSqlQuery snap(db);
        snap.prepare(QStringLiteral(
            "UPDATE detail_commande SET "
            "nom_plat_snapshot = COALESCE(NULLIF(TRIM(nom_plat_snapshot), ''), ?), "
            "nom_menu_snapshot = COALESCE(NULLIF(TRIM(nom_menu_snapshot), ''), ?) "
            "WHERE id_menu = ?"));
        snap.addBindValue(nomPlat);
        snap.addBindValue(nomPlat);
        snap.addBindValue(idMenu);
        snap.exec();
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM menu_jour WHERE id_menu = ?"));
    query.addBindValue(idMenu);

    if (!query.exec()) {
        const QSqlError err = query.lastError();
        if (err.nativeErrorCode() == QStringLiteral("1451")) {
            setError(error, QStringLiteral(
                "Suppression impossible : ce menu est toujours référencé par des commandes.\n"
                "Les données historiques sont conservées grâce aux informations enregistrées."));
        } else {
            setError(error, QStringLiteral("Impossible de retirer le menu : %1").arg(err.text()));
        }
        return false;
    }
    if (query.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Menu introuvable."));
        return false;
    }
    return true;
}

int MenuManager::compterParDate(const QDate &date, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM menu_jour WHERE date_menu = ?"));
    query.addBindValue(date.toString(QStringLiteral("yyyy-MM-dd")));
    if (!query.exec() || !query.next()) {
        setError(error, QStringLiteral("Impossible de compter les menus : %1")
                            .arg(query.lastError().text()));
        return 0;
    }
    return query.value(0).toInt();
}

int MenuManager::quantiteRestante(int idMenu, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT (m.quantite_initiale - COALESCE(("
        "  SELECT SUM(dc.quantite) FROM detail_commande dc "
        "  INNER JOIN commande c ON c.id_commande = dc.id_commande "
        "  WHERE dc.id_menu = m.id_menu AND c.statut = 'VALIDEE'), 0)) "
        "FROM menu_jour m WHERE m.id_menu = ?"));
    query.addBindValue(idMenu);
    if (!query.exec() || !query.next()) {
        setError(error, QStringLiteral("Menu introuvable."));
        return -1;
    }
    return query.value(0).toInt();
}
