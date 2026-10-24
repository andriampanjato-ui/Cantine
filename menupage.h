#ifndef MENUPAGE_H
#define MENUPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QPushButton>

class MenuManager;
class PlatManager;

class MenuPage : public QWidget
{
    Q_OBJECT

public:
    explicit MenuPage(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onSelectionChanged();
    void onAjouter();
    void onModifier();
    void onSupprimer();
    void onVider();

private:
    void chargerPlats();
    void viderFormulaire();
    int idSelectionne() const;

    MenuManager *m_menuManager;
    PlatManager *m_platManager;

    QDateEdit *m_dateFiltre;
    QTableWidget *m_table;

    QComboBox *m_cboPlat;
    QDateEdit *m_dateMenu;
    QSpinBox *m_spinQuantite;
    QPushButton *m_btnModifier;
    QPushButton *m_btnSupprimer;
};

#endif // MENUPAGE_H
