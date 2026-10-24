#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <QObject>
#include <QList>
#include <QString>

struct ClientData
{
    int id = 0;
    QString nom;
    QString telephone;
};

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QObject *parent = nullptr);

    QList<ClientData> getAll(const QString &search = QString(), QString *error = nullptr) const;
    bool ajouter(const QString &nom, const QString &telephone, QString *error = nullptr);
    bool modifier(int idClient, const QString &nom, const QString &telephone, QString *error = nullptr);
    bool supprimer(int idClient, QString *error = nullptr);
    int compter(QString *error = nullptr) const;
    QString nomClient(int idClient, QString *error = nullptr) const;
};

#endif // CLIENTMANAGER_H
