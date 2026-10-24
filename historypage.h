#ifndef HISTORYPAGE_H
#define HISTORYPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

class CommandeManager;

class HistoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPage(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onSelectionChanged();
    void onValider();
    void onAnnuler();

private:
    int idSelectionne() const;
    QString statutSelectionne() const;
    void afficherDetails(int idCommande, const QString &nomClient, const QString &statut,
                         double montant);

    CommandeManager *m_manager;
    QLineEdit *m_txtRecherche;
    QComboBox *m_cboStatut;
    QTableWidget *m_table;
    QTableWidget *m_tableDetails;
    QLabel *m_lblDetails;
    QLabel *m_lblAucunResultat;
    QPushButton *m_btnValider;
    QPushButton *m_btnAnnuler;
    QTimer *m_timerRecherche;
};

#endif // HISTORYPAGE_H
