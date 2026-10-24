#ifndef CLIENTSPAGE_H
#define CLIENTSPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>

class ClientManager;

class ClientsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ClientsPage(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onSelectionChanged();
    void onAjouter();
    void onModifier();
    void onSupprimer();
    void onVider();

private:
    void viderFormulaire();
    int idSelectionne() const;

    ClientManager *m_manager;
    QLineEdit *m_txtRecherche;
    QTableWidget *m_table;
    QLineEdit *m_txtNom;
    QLineEdit *m_txtTelephone;
    QPushButton *m_btnModifier;
    QPushButton *m_btnSupprimer;
};

#endif // CLIENTSPAGE_H
