#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>

class ClientManager;
class PlatManager;
class MenuManager;
class CommandeManager;

class Dashboard : public QWidget
{
    Q_OBJECT

public:
    explicit Dashboard(QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    QWidget *creerCarte(const QString &titre, const QString &symbole, QLabel **valeurLabel);

    ClientManager *m_clientManager;
    PlatManager *m_platManager;
    MenuManager *m_menuManager;
    CommandeManager *m_commandeManager;

    QLabel *m_lblClients;
    QLabel *m_lblPlats;
    QLabel *m_lblMenusJour;
    QLabel *m_lblCommandesValidees;
    QLabel *m_lblChiffreAffaires;

    QTableWidget *m_tableRecentes;
    QTableWidget *m_tableStockFaible;
    QLabel *m_lblDate;
};

#endif // DASHBOARD_H
