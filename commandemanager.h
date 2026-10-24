#ifndef COMMANDEMANAGER_H
#define COMMANDEMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QDateTime>

struct LigneCommande
{
    int idMenu = 0;
    int idPlat = 0;
    QString nomPlat;
    int quantite = 0;
    double prixUnitaire = 0.0;
    double sousTotal = 0.0;
};

struct CommandeData
{
    int id = 0;
    int idClient = 0;
    QString nomClient;
    QDateTime dateCommande;
    QString statut;
    double montantTotal = 0.0;
    int nbLignes = 0;
};

class CommandeManager : public QObject
{
    Q_OBJECT
public:
    explicit CommandeManager(QObject *parent = nullptr);

    int enregistrerCommande(int idClient,
                            const QDateTime &dateCommande,
                            const QList<LigneCommande> &lignes,
                            bool valider,
                            QString *error = nullptr);

    bool validerCommande(int idCommande, QString *error = nullptr);
    bool annulerCommande(int idCommande, QString *error = nullptr);

    QList<CommandeData> getHistorique(const QString &recherche = QString(),
                                      const QString &statut = QString(),
                                      QString *error = nullptr) const;
    QList<LigneCommande> getDetails(int idCommande, QString *error = nullptr) const;

    int compterValidees(const QDate &jour = QDate(), QString *error = nullptr) const;
    double chiffreAffaires(const QDate &jour = QDate(), QString *error = nullptr) const;
    double montantCommande(int idCommande, QString *error = nullptr) const;
    QList<CommandeData> getRecentes(int limite = 5, QString *error = nullptr) const;

private:
    bool validerInterne(int idCommande, QString *error);
};

#endif // COMMANDEMANAGER_H
