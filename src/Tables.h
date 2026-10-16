#pragma once

#include <QString>
#include <QVector>

// Les tables où l'on peut s'asseoir, et jusqu'où la partie va.
//
// Cinq tables, et ce qui change d'une à l'autre n'est ni le nombre de joueurs
// -- la belote se joue à quatre et il n'y a rien à y faire -- ni les règles,
// mais la longueur de la partie. Ce n'est pas un détail de réglage : à trois
// cent un points une donne mal prise coûte la partie, et le jeu à y jouer est
// un jeu prudent ; à mille, il y a le temps de tenter une prise sur cinq
// cartes et de se refaire, et le temps aussi pour que ce que les six
// adversaires ont appris finisse par se voir.
//
// Elles sont toutes du même drap : la couleur du tapis est un goût, elle se
// choisit dans les réglages et appartient au profil -- voir TableLook.h. Une
// partie à mille n'est pas d'une autre couleur qu'une partie à trois cent un,
// elle est seulement plus longue, et c'est le nombre de donnes écrit dans le
// coin de la tuile qui le dit.
struct TableKind {
    QString id;
    QString name;
    QString blurb;
    int points;      // le but ; zéro pour la table d'une seule donne
};

const QVector<TableKind> &tableKinds();
const TableKind &tableKindById(const QString &id);
QString defaultTableId();

// Combien de donnes il faut, à peu près, pour aller jusqu'à `points`. Une
// donne distribue cent soixante-deux points entre les deux camps et c'est
// rarement moitié-moitié : un camp en prend quatre-vingt-dix en moyenne, et
// c'est de là que vient le chiffre. Zéro pour la table d'une seule donne, qui
// est déjà comptée en donnes.
int donnesFor(int points);
