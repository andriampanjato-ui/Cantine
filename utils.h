#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QLocale>
#include <QDate>
#include <QDateTime>

inline QString formatAr(double valeur)
{
    QLocale locale(QLocale::French, QLocale::Madagascar);
    return locale.toString(valeur, 'f', 2) + QStringLiteral(" Ar");
}

inline QString formatDate(const QDate &date)
{
    return date.toString(QStringLiteral("dd/MM/yyyy"));
}

inline QString formatDateTime(const QDateTime &dateTime)
{
    return dateTime.toString(QStringLiteral("dd/MM/yyyy HH:mm"));
}

inline QString formaterMontant(double montant)
{
    return QString::number(montant, 'f', 2) + QStringLiteral(" Ar");
}

inline QString statutVersLibelle(const QString &statut)
{
    if (statut == QStringLiteral("VALIDEE")) {
        return QStringLiteral("Validée");
    }
    if (statut == QStringLiteral("ANNULEE")) {
        return QStringLiteral("Annulée");
    }
    return QStringLiteral("En attente");
}

#endif // UTILS_H
