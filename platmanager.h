#ifndef PLATMANAGER_H
#define PLATMANAGER_H

#include <QObject>
#include <QList>
#include <QString>

struct PlatData
{
    int id = 0;
    QString nom;
    double prix = 0.0;
};

class PlatManager : public QObject
{
    Q_OBJECT
public:
    explicit PlatManager(QObject *parent = nullptr);

    QList<PlatData> getAll(const QString &search = QString(), QString *error = nullptr) const;
    bool ajouter(const QString &nom, double prix, QString *error = nullptr);
    bool modifier(int idPlat, const QString &nom, double prix, QString *error = nullptr);
    bool supprimer(int idPlat, QString *error = nullptr);
    int compter(QString *error = nullptr) const;
};

#endif // PLATMANAGER_H
