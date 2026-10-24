#ifndef NEWORDERPAGE_H
#define NEWORDERPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QTableWidget>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QDate>
#include "commandemanager.h"

class ClientManager;
class MenuManager;

class NewOrderPage : public QWidget
{
    Q_OBJECT

public:
    explicit NewOrderPage(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onDateChanged(const QDate &date);
    void onAjouterLigne();
    void onRetirerLigne();
    void onVider();
    void onEnregistrerEnAttente();
    void onValiderCommande();

private:
    void refreshClients();
    void refreshMenus();
    void afficherLignes();
    void recalculerTotal();
    int quantiteDejaAjoutee(int idMenu) const;
    bool enregistrer(bool valider);

    ClientManager *m_clientManager;
    MenuManager *m_menuManager;
    CommandeManager *m_commandeManager;

    QComboBox *m_cboClient;
    QDateTimeEdit *m_dateCommande;
    QTableWidget *m_tableMenus;
    QTableWidget *m_tableLignes;
    QSpinBox *m_spinQuantite;
    QLabel *m_lblTotal;
    QLabel *m_lblInfo;
    QTableWidgetItem *m_selectionMenu;

    QList<LigneCommande> m_lignes;
    QDate m_dateMenusCourante;
};

#endif // NEWORDERPAGE_H
