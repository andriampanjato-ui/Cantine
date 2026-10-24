#include "commandemanager.h"
#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSqlDatabase>
#include <QMap>
#include <QRegularExpression>
#include <QStringList>

namespace {

void setError(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
}

QString echapperLike(QString valeur)
{
    valeur.replace(QLatin1Char('!'), QStringLiteral("!!"));
    valeur.replace(QLatin1Char('%'), QStringLiteral("!%"));
    valeur.replace(QLatin1Char('_'), QStringLiteral("!_"));
    return valeur;
}

bool demarrerTransaction(QString *error)
{
    QSqlQuery query(Database::instance()->getDatabase());
    if (!query.exec(QStringLiteral("SET TRANSACTION ISOLATION LEVEL READ COMMITTED"))) {
        setError(error, QStringLiteral("Impossible de configurer la transaction : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    if (!Database::instance()->getDatabase().transaction()) {
        setError(error, QStringLiteral("Impossible de démarrer la transaction : %1")
                            .arg(Database::instance()->getDatabase().lastError().text()));
        return false;
    }
    return true;
}

bool validerTransaction(QString *error)
{
    if (!Database::instance()->getDatabase().commit()) {
        setError(error, QStringLiteral("Impossible de valider la transaction : %1")
                            .arg(Database::instance()->getDatabase().lastError().text()));
        return false;
    }
    return true;
}

void annulerTransaction()
{
    Database::instance()->getDatabase().rollback();
}

bool verrouillerMenus(const QList<int> &idsMenu, QString *error)
{
    if (idsMenu.isEmpty()) {
        return true;
    }
    QSqlQuery query(Database::instance()->getDatabase());
    QString sql = QStringLiteral("SELECT id_menu, quantite_initiale FROM menu_jour WHERE id_menu IN (");
    for (int i = 0; i < idsMenu.size(); ++i) {
        sql += QStringLiteral("?");
        if (i < idsMenu.size() - 1) {
            sql += QStringLiteral(",");
        }
    }
    sql += QStringLiteral(") FOR UPDATE");
    query.prepare(sql);
    for (int id : idsMenu) {
        query.addBindValue(id);
    }
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de verrouiller les menus : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    return true;
}

int quantiteCommandeeVerouillee(int idMenu, QString *error)
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(dc.quantite), 0) "
        "FROM detail_commande dc "
        "INNER JOIN commande c ON c.id_commande = dc.id_commande "
        "WHERE dc.id_menu = ? AND c.statut = 'VALIDEE' "
        "FOR UPDATE"));
    query.addBindValue(idMenu);
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de vérifier les quantités commandées : %1")
                            .arg(query.lastError().text()));
        return -1;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool recupererMenu(int idMenu, int *quantiteInitiale, double *prixActuel, QString *error)
{
    if (!quantiteInitiale || !prixActuel) {
        setError(error, QStringLiteral("Erreur interne : paramètres manquants."));
        return false;
    }
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT m.quantite_initiale, p.prix "
        "FROM menu_jour m "
        "LEFT JOIN plat p ON p.id_plat = m.id_plat "
        "WHERE m.id_menu = ?"));
    query.addBindValue(idMenu);
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de récupérer le menu : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    if (!query.next()) {
        setError(error, QStringLiteral("Menu introuvable."));
        return false;
    }
    *quantiteInitiale = query.value(0).toInt();
    *prixActuel = query.value(1).toDouble();
    return true;
}

bool clientExiste(int idClient)
{
    if (idClient <= 0) {
        return false;
    }
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("SELECT 1 FROM client WHERE id_client = ?"));
    query.addBindValue(idClient);
    return query.exec() && query.next();
}

} // namespace

CommandeManager::CommandeManager(QObject *parent)
    : QObject(parent)
{
}

int CommandeManager::enregistrerCommande(int idClient,
                                        const QDateTime &dateCommande,
                                        const QList<LigneCommande> &lignes,
                                        bool valider,
                                        QString *error)
{
    if (idClient <= 0) {
        setError(error, QStringLiteral("Sélectionnez un client."));
        return -1;
    }
    if (!clientExiste(idClient)) {
        setError(error, QStringLiteral("Client introuvable."));
        return -1;
    }
    if (!dateCommande.isValid()) {
        setError(error, QStringLiteral("Date de commande invalide."));
        return -1;
    }
    if (lignes.isEmpty()) {
        setError(error, QStringLiteral("Ajoutez au moins un plat à la commande."));
        return -1;
    }

    QSqlDatabase db = Database::instance()->getDatabase();
    if (!db.isOpen()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return -1;
    }

    if (!demarrerTransaction(error)) {
        return -1;
    }

    QList<int> idsMenu;
    idsMenu.reserve(lignes.size());
    for (const LigneCommande &ligne : lignes) {
        if (ligne.idMenu <= 0 || ligne.quantite <= 0) {
            annulerTransaction();
            setError(error, QStringLiteral("Chaque ligne doit avoir un menu et une quantité valide."));
            return -1;
        }
        idsMenu.append(ligne.idMenu);
    }
    std::sort(idsMenu.begin(), idsMenu.end());
    idsMenu.erase(std::unique(idsMenu.begin(), idsMenu.end()), idsMenu.end());
    if (!verrouillerMenus(idsMenu, error)) {
        annulerTransaction();
        return -1;
    }

    QMap<int, int> quantitesParMenu;
    for (const LigneCommande &ligne : lignes) {
        quantitesParMenu[ligne.idMenu] += ligne.quantite;
    }

    for (auto it = quantitesParMenu.constBegin(); it != quantitesParMenu.constEnd(); ++it) {
        int quantiteInitiale = 0;
        double prixActuel = 0.0;
        if (!recupererMenu(it.key(), &quantiteInitiale, &prixActuel, error)) {
            annulerTransaction();
            return -1;
        }
        const int dejaCommandee = quantiteCommandeeVerouillee(it.key(), error);
        if (dejaCommandee < 0) {
            annulerTransaction();
            return -1;
        }
        const int restante = quantiteInitiale - dejaCommandee;
        if (it.value() > restante) {
            annulerTransaction();
            setError(error, QStringLiteral("Quantité insuffisante pour le menu sélectionné."));
            return -1;
        }
    }

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

    int idCommande = 0;
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT INTO commande (id_client, nom_client_snapshot, telephone_client_snapshot, date_commande, statut) "
            "VALUES (?, ?, ?, ?, 'EN_ATTENTE')"));
        query.addBindValue(idClient);
        query.addBindValue(nomClient);
        query.addBindValue(telephoneClient);
        query.addBindValue(dateCommande.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        if (!query.exec()) {
            annulerTransaction();
            setError(error, QStringLiteral("Impossible de créer la commande : %1")
                                .arg(query.lastError().text()));
            return -1;
        }
        idCommande = query.lastInsertId().toInt();
    }

    for (const LigneCommande &ligne : lignes) {
        int quantiteInitiale = 0;
        double prixActuel = 0.0;
        if (!recupererMenu(ligne.idMenu, &quantiteInitiale, &prixActuel, error)) {
            annulerTransaction();
            return -1;
        }
        if (prixActuel < 0.0) {
            prixActuel = 0.0;
        }
        QString nomPlatSnapshot;
        QString nomMenuSnapshot;
        {
            QSqlQuery infoMenu(db);
            infoMenu.prepare(QStringLiteral(
                "SELECT p.nom_plat FROM menu_jour m LEFT JOIN plat p ON p.id_plat = m.id_plat WHERE m.id_menu = ?"));
            infoMenu.addBindValue(ligne.idMenu);
            if (infoMenu.exec() && infoMenu.next()) {
                nomPlatSnapshot = infoMenu.value(0).toString();
                nomMenuSnapshot = nomPlatSnapshot;
            }
        }
        QSqlQuery queryDetail(db);
        queryDetail.prepare(QStringLiteral(
            "INSERT INTO detail_commande (id_commande, id_menu, nom_plat_snapshot, nom_menu_snapshot, quantite, prix_unitaire) "
            "VALUES (?, ?, ?, ?, ?, ?)"));
        queryDetail.addBindValue(idCommande);
        queryDetail.addBindValue(ligne.idMenu);
        queryDetail.addBindValue(nomPlatSnapshot);
        queryDetail.addBindValue(nomMenuSnapshot);
        queryDetail.addBindValue(ligne.quantite);
        queryDetail.addBindValue(prixActuel);
        if (!queryDetail.exec()) {
            annulerTransaction();
            setError(error, QStringLiteral("Impossible d'ajouter le détail de commande : %1")
                                .arg(queryDetail.lastError().text()));
            return -1;
        }
    }

    if (valider) {
        if (!validerInterne(idCommande, error)) {
            annulerTransaction();
            return -1;
        }
    }

    if (!validerTransaction(error)) {
        annulerTransaction();
        return -1;
    }
    return idCommande;
}

bool CommandeManager::validerCommande(int idCommande, QString *error)
{
    if (idCommande <= 0) {
        setError(error, QStringLiteral("Aucune commande sélectionnée."));
        return false;
    }
    if (!demarrerTransaction(error)) {
        return false;
    }
    if (!validerInterne(idCommande, error)) {
        annulerTransaction();
        return false;
    }
    if (!validerTransaction(error)) {
        annulerTransaction();
        return false;
    }
    return true;
}

bool CommandeManager::validerInterne(int idCommande, QString *error)
{
    QSqlQuery statutQuery(Database::instance()->getDatabase());
    statutQuery.prepare(QStringLiteral("SELECT statut FROM commande WHERE id_commande = ? FOR UPDATE"));
    statutQuery.addBindValue(idCommande);
    if (!statutQuery.exec()) {
        setError(error, QStringLiteral("Impossible de vérifier la commande : %1")
                            .arg(statutQuery.lastError().text()));
        return false;
    }
    if (!statutQuery.next()) {
        setError(error, QStringLiteral("Commande introuvable."));
        return false;
    }
    const QString statut = statutQuery.value(0).toString();
    if (statut == QStringLiteral("VALIDEE")) {
        return true;
    }
    if (statut == QStringLiteral("ANNULEE")) {
        setError(error, QStringLiteral("Impossible de valider une commande annulée."));
        return false;
    }

    QList<QPair<int, int> > lignes;
    {
        QSqlQuery lignesQuery(Database::instance()->getDatabase());
        lignesQuery.prepare(QStringLiteral(
            "SELECT id_menu, quantite FROM detail_commande WHERE id_commande = ?"));
        lignesQuery.addBindValue(idCommande);
        if (!lignesQuery.exec()) {
            setError(error, QStringLiteral("Impossible de lire les détails de la commande : %1")
                                .arg(lignesQuery.lastError().text()));
            return false;
        }
        QList<int> idsMenu;
        while (lignesQuery.next()) {
            const int idMenu = lignesQuery.value(0).toInt();
            const int quantite = lignesQuery.value(1).toInt();
            if (idMenu > 0) {
                lignes.append(qMakePair(idMenu, quantite));
                idsMenu.append(idMenu);
            }
        }
        std::sort(idsMenu.begin(), idsMenu.end());
        idsMenu.erase(std::unique(idsMenu.begin(), idsMenu.end()), idsMenu.end());
        if (!verrouillerMenus(idsMenu, error)) {
            return false;
        }
    }

    QMap<int, int> totalParMenu;
    for (const auto &ligne : lignes) {
        if (ligne.first > 0) {
            totalParMenu[ligne.first] += ligne.second;
        }
    }
    for (auto it = totalParMenu.constBegin(); it != totalParMenu.constEnd(); ++it) {
        int quantiteInitiale = 0;
        double prixActuel = 0.0;
        if (!recupererMenu(it.key(), &quantiteInitiale, &prixActuel, error)) {
            return false;
        }
        const int dejaCommandee = quantiteCommandeeVerouillee(it.key(), error);
        if (dejaCommandee < 0) {
            return false;
        }
        const int restante = quantiteInitiale - dejaCommandee;
        if (it.value() > restante) {
            setError(error, QStringLiteral("Quantité insuffisante pour valider cette commande."));
            return false;
        }
    }

    QSqlQuery updateQuery(Database::instance()->getDatabase());
    updateQuery.prepare(QStringLiteral("UPDATE commande SET statut = 'VALIDEE' WHERE id_commande = ? AND statut = 'EN_ATTENTE'"));
    updateQuery.addBindValue(idCommande);
    if (!updateQuery.exec()) {
        setError(error, QStringLiteral("Impossible de valider la commande : %1")
                            .arg(updateQuery.lastError().text()));
        return false;
    }
    if (updateQuery.numRowsAffected() == 0) {
        setError(error, QStringLiteral("La commande n'a pas pu être validée (statut modifié entre-temps)."));
        return false;
    }
    return true;
}

bool CommandeManager::annulerCommande(int idCommande, QString *error)
{
    if (idCommande <= 0) {
        setError(error, QStringLiteral("Aucune commande sélectionnée."));
        return false;
    }
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("UPDATE commande SET statut = 'ANNULEE' WHERE id_commande = ?"));
    query.addBindValue(idCommande);
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible d'annuler la commande : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    if (query.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Commande introuvable."));
        return false;
    }
    return true;
}

QList<CommandeData> CommandeManager::getHistorique(const QString &recherche, const QString &statut, QString *error) const
{
    QList<CommandeData> result;
    QSqlQuery query(Database::instance()->getDatabase());

    QString sql = QStringLiteral(
        "SELECT c.id_commande, c.id_client, "
        "       COALESCE(NULLIF(TRIM(c.nom_client_snapshot), ''), cl.nom, CONCAT('Client #', COALESCE(c.id_client,0))) AS nom_client_aff, "
        "       c.date_commande, c.statut, "
        "       COALESCE(SUM(dc.quantite * dc.prix_unitaire), 0) AS montant, "
        "       COUNT(dc.id_detail) AS nb_lignes "
        "FROM commande c "
        "LEFT JOIN client cl ON cl.id_client = c.id_client "
        "LEFT JOIN detail_commande dc ON dc.id_commande = c.id_commande "
        "WHERE 1 = 1 ");

    QStringList champs;
    QVariantList valeurs;

    const QString terme = recherche.trimmed();
    if (!terme.isEmpty()) {
        const QString pattern = QStringLiteral("%") + echapperLike(terme) + QStringLiteral("%");
        bool rechercheParNumero = false;
        if (terme.startsWith(QLatin1Char('#'))) {
            const QString num = terme.mid(1).trimmed();
            bool ok = false;
            const int idCible = num.toInt(&ok);
            if (ok && idCible > 0) {
                champs << QStringLiteral("c.id_commande = ?");
                valeurs << idCible;
                rechercheParNumero = true;
            }
        } else {
            bool ok = false;
            const int idCible = terme.toInt(&ok);
            if (ok && idCible > 0 && terme == QString::number(idCible)) {
                champs << QStringLiteral("c.id_commande = ?");
                valeurs << idCible;
                rechercheParNumero = true;
            }
        }
        if (!rechercheParNumero) {
            champs << QStringLiteral("(COALESCE(NULLIF(TRIM(c.nom_client_snapshot), ''), cl.nom) LIKE ? ESCAPE '!' OR COALESCE(c.telephone_client_snapshot, cl.telephone, '') LIKE ? ESCAPE '!')");
            valeurs << pattern << pattern;
        }
    }

    if (!statut.isEmpty()) {
        champs << QStringLiteral("c.statut = ?");
        valeurs << statut;
    }

    for (const QString &champ : champs) {
        sql += QStringLiteral(" AND ") + champ;
    }

    sql += QStringLiteral(
        " GROUP BY c.id_commande, c.id_client, c.nom_client_snapshot, c.telephone_client_snapshot, cl.nom, c.date_commande, c.statut "
        " ORDER BY c.date_commande DESC, c.id_commande DESC");

    query.prepare(sql);
    for (const QVariant &v : valeurs) {
        query.addBindValue(v);
    }

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger l'historique : %1")
                            .arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        CommandeData commande;
        commande.id = query.value(0).toInt();
        commande.idClient = query.value(1).toInt();
        commande.clientExistant = !query.isNull(1);
        commande.nomClient = query.value(2).toString();
        commande.dateCommande = query.value(3).toDateTime();
        commande.statut = query.value(4).toString();
        commande.montantTotal = query.value(5).toDouble();
        commande.nbLignes = query.value(6).toInt();
        result.append(commande);
    }
    return result;
}

QList<LigneCommande> CommandeManager::getDetails(int idCommande, QString *error) const
{
    QList<LigneCommande> result;
    if (idCommande <= 0) {
        setError(error, QStringLiteral("Commande invalide."));
        return result;
    }
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT dc.id_menu, m.id_plat, COALESCE(NULLIF(TRIM(dc.nom_plat_snapshot), ''), p.nom_plat, '') AS nom_plat_aff, dc.quantite, dc.prix_unitaire "
        "FROM detail_commande dc "
        "LEFT JOIN menu_jour m ON m.id_menu = dc.id_menu "
        "LEFT JOIN plat p ON p.id_plat = m.id_plat "
        "WHERE dc.id_commande = ? "
        "ORDER BY nom_plat_aff ASC"));
    query.addBindValue(idCommande);
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger le détail de la commande : %1")
                            .arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        LigneCommande ligne;
        ligne.idMenu = query.value(0).toInt();
        ligne.idPlat = query.value(1).toInt();
        ligne.nomPlat = query.value(2).toString();
        ligne.quantite = query.value(3).toInt();
        ligne.prixUnitaire = query.value(4).toDouble();
        result.append(ligne);
    }
    return result;
}

int CommandeManager::compterValidees(const QDate &jour, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    QString sql = QStringLiteral("SELECT COUNT(*) FROM commande WHERE statut = 'VALIDEE'");
    if (jour.isValid()) {
        sql += QStringLiteral(" AND DATE(date_commande) = ?");
        query.prepare(sql);
        query.addBindValue(jour.toString(QStringLiteral("yyyy-MM-dd")));
    } else {
        query.prepare(sql);
    }
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de compter les commandes validées : %1")
                            .arg(query.lastError().text()));
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

double CommandeManager::chiffreAffaires(const QDate &jour, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    QString sql = QStringLiteral(
        "SELECT COALESCE(SUM(dc.quantite * dc.prix_unitaire), 0) "
        "FROM detail_commande dc "
        "INNER JOIN commande c ON c.id_commande = dc.id_commande "
        "WHERE c.statut = 'VALIDEE'");
    if (jour.isValid()) {
        sql += QStringLiteral(" AND DATE(c.date_commande) = ?");
        query.prepare(sql);
        query.addBindValue(jour.toString(QStringLiteral("yyyy-MM-dd")));
    } else {
        query.prepare(sql);
    }
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de calculer le chiffre d'affaires : %1")
                            .arg(query.lastError().text()));
        return 0.0;
    }
    if (query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
}

double CommandeManager::montantCommande(int idCommande, QString *error) const
{
    if (idCommande <= 0) {
        setError(error, QStringLiteral("Commande invalide."));
        return 0.0;
    }
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(quantite * prix_unitaire), 0) "
        "FROM detail_commande WHERE id_commande = ?"));
    query.addBindValue(idCommande);
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de calculer le montant : %1")
                            .arg(query.lastError().text()));
        return 0.0;
    }
    if (query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
}

QList<CommandeData> CommandeManager::getRecentes(int limite, QString *error) const
{
    QList<CommandeData> result;
    if (limite <= 0) {
        limite = 5;
    }
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT c.id_commande, c.id_client, "
        "       COALESCE(NULLIF(TRIM(c.nom_client_snapshot), ''), cl.nom, CONCAT('Client #', COALESCE(c.id_client,0))) AS nom_client_aff, "
        "       c.date_commande, c.statut, "
        "       COALESCE(SUM(dc.quantite * dc.prix_unitaire), 0) AS montant, "
        "       COUNT(dc.id_detail) AS nb_lignes "
        "FROM commande c "
        "LEFT JOIN client cl ON cl.id_client = c.id_client "
        "LEFT JOIN detail_commande dc ON dc.id_commande = c.id_commande "
        "GROUP BY c.id_commande, c.id_client, c.nom_client_snapshot, cl.nom, c.date_commande, c.statut "
        "ORDER BY c.date_commande DESC, c.id_commande DESC "
        "LIMIT ?"));
    query.addBindValue(limite);
    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger les commandes récentes : %1")
                            .arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        CommandeData commande;
        commande.id = query.value(0).toInt();
        commande.idClient = query.value(1).toInt();
        commande.clientExistant = !query.isNull(1);
        commande.nomClient = query.value(2).toString();
        commande.dateCommande = query.value(3).toDateTime();
        commande.statut = query.value(4).toString();
        commande.montantTotal = query.value(5).toDouble();
        commande.nbLignes = query.value(6).toInt();
        result.append(commande);
    }
    return result;
}

bool CommandeManager::supprimerCommande(int idCommande, QString *error)
{
    if (idCommande <= 0) {
        setError(error, QStringLiteral("Aucune commande sélectionnée."));
        return false;
    }

    QSqlDatabase db = Database::instance()->getDatabase();
    if (!db.isOpen()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return false;
    }

    if (!demarrerTransaction(error)) {
        return false;
    }

    QSqlQuery verif(db);
    verif.prepare(QStringLiteral("SELECT id_commande FROM commande WHERE id_commande = ? FOR UPDATE"));
    verif.addBindValue(idCommande);
    if (!verif.exec() || !verif.next()) {
        annulerTransaction();
        setError(error, QStringLiteral("Commande introuvable."));
        return false;
    }

    // Supprimer les détails d'abord si nécessaire (même si ON DELETE CASCADE est présent)
    {
        QSqlQuery delDetail(db);
        delDetail.prepare(QStringLiteral("DELETE FROM detail_commande WHERE id_commande = ?"));
        delDetail.addBindValue(idCommande);
        if (!delDetail.exec()) {
            annulerTransaction();
            setError(error, QStringLiteral("Impossible de supprimer les détails de la commande : %1")
                                .arg(delDetail.lastError().text()));
            return false;
        }
    }

    QSqlQuery delCmd(db);
    delCmd.prepare(QStringLiteral("DELETE FROM commande WHERE id_commande = ?"));
    delCmd.addBindValue(idCommande);
    if (!delCmd.exec()) {
        annulerTransaction();
        setError(error, QStringLiteral("Impossible de supprimer la commande : %1")
                            .arg(delCmd.lastError().text()));
        return false;
    }

    if (!validerTransaction(error)) {
        annulerTransaction();
        return false;
    }
    return true;
}
