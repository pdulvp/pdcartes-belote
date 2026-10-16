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
class QToolButton;

// Les réglages : la façon de jouer, trois ou quatre commodités pendant le jeu,
// les cartes avec lesquelles on joue, et la couleur du drap.
//
// Ce qui appartient à un profil ou à un joueur en particulier n'est plus ici
// mais sur la chose elle-même, au menu d'ouverture : on renomme un profil
// depuis sa plaque dans la bande du haut, et on remet un joueur à son niveau
// de départ d'un clic droit sur sa carte. Les réglages ne gardent que ce qui
// vaut pour toute la partie -- ce qui les ramène à une page qui se lit d'un
// coup d'œil.
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
    void describeVariant();
    void dressFelts();
    void fillDecks();
    void fillBacks();
    void describeDeck();

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
