#pragma once

#include "TablePage.h"

#include <QDialog>
#include <QVector>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QSlider;
class QStackedWidget;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;

// Les réglages : la façon de jouer, les commodités, les cartes, et le tapis.
//
// Quatre pages plutôt qu'une seule colonne qui descend. Tout tenait dans une
// seule page haute de huit cents pixels, ce qui était deux choses à la fois :
// une page qu'un écran d'ordinateur portable ne montre pas en entier, et une
// page où l'on cherche un réglage en faisant glisser. Les quatre groupes
// existaient déjà -- ils avaient leur titre et leur filet -- ils sont
// simplement devenus quatre pages, et la liste de gauche dit lesquelles il y a.
//
// Ce qui appartient à un profil ou à un joueur en particulier n'est pas ici
// mais sur la chose elle-même, au menu d'ouverture : on renomme un profil
// depuis sa plaque dans la bande du haut, et on remet un joueur à son niveau de
// départ d'un clic droit sur sa carte.
//
// La longueur de la partie n'est pas ici non plus : elle est écrite sur les
// tuiles du menu, parce qu'à la belote c'est elle qui fait la table. Un réglage
// caché dans une boîte aurait voulu dire que les cinq tables sont la même
// table.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(const GameSettings &settings, QWidget *parent = nullptr);

    GameSettings settings() const;

private:
    // Une page, et sa ligne dans la liste de gauche. Rend le layout vertical à
    // remplir : le titre et le ressort du bas sont déjà posés.
    QVBoxLayout *addPage(const QString &name, const QString &heading);

    void buildPlayPage();
    void buildDuringPage();
    void buildCardsPage();
    void buildFeltPage();

    void describeVariant();
    void dressFelts();
    // Les deux listes portent leur propre feuille de style, qui est écrite
    // avec les couleurs du moment : la teinte s'essayant en direct, il faut
    // pouvoir la réécrire.
    void dressLists();
    void fillDecks();
    void fillBacks();
    void describeDeck();

    QTreeWidget *m_sections = nullptr;
    QStackedWidget *m_pages = nullptr;

    QButtonGroup *m_variants = nullptr;
    QButtonGroup *m_paces = nullptr;
    QCheckBox *m_annonces = nullptr;
    QLabel *m_variantNote = nullptr;
    QCheckBox *m_holdByStrength = nullptr;
    QCheckBox *m_offerHints = nullptr;
    QCheckBox *m_celebrate = nullptr;
    QComboBox *m_deck = nullptr;
    QSlider *m_zoom = nullptr;
    QLabel *m_zoomNote = nullptr;
    QListWidget *m_backs = nullptr;
    QLabel *m_deckNote = nullptr;
    QVector<QToolButton *> m_felts;
    GameSettings m_settings;
};
