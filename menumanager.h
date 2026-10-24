#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QDate>
#include <QVariant>

struct MenuData
{
    int idMenu = 0;
    int idPlat = 0;
    QString nomPlat;
    double prix = 0.0;
    QDate dateMenu;
    int quantiteInitiale = 0;
    int quantiteCommandee = 0;
    int quantiteRestante = 0;
};

class MenuManager : public QObject
{
    Q_OBJECT
public:
    explicit MenuManager(QObject *parent = nullptr);

    QList<MenuData> getMenusParDate(const QDate &date, QString *error = nullptr) const;
    QList<MenuData> getMenusDisponibles(const QDate &date, QString *error = nullptr) const;
    QList<MenuData> getMenusStockFaible(int seuil, QString *error = nullptr) const;
    bool ajouter(int idPlat, const QDate &dateMenu, int quantiteInitiale, QString *error = nullptr);
    bool modifier(int idMenu, int quantiteInitiale, QString *error = nullptr);
    bool supprimer(int idMenu, QString *error = nullptr);
    int compterParDate(const QDate &date, QString *error = nullptr) const;
    int quantiteRestante(int idMenu, QString *error = nullptr) const;

private:
    QList<MenuData> chargerMenus(const QString &whereClause,
                                 const QList<QVariant> &params,
                                 const QString &havingClause,
                                 const QString &orderClause,
                                 QString *error) const;
};

#endif // MENUMANAGER_H
