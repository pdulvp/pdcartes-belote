#pragma once

#include "Belote.h"
#include "Cards.h"
#include "Rivals.h"

#include <QString>
#include <QVector>

// Comment un adversaire décide.
//
// Un niveau n'est pas un nombre qu'on multiplie quelque part. C'est une liste
// de choses qu'un joueur fait ou ne fait pas, et chacune d'elles est une chose
// qu'on peut voir arriver de l'autre côté de la table :
//
//   prendre ou passer   Un débutant compte les points qu'il a dans la couleur
//                       de la retourne et prend s'il y en a. Il ne voit pas
//                       que trois atouts sans le valet ne tiendront rien, ni
//                       que le roi et la dame d'atout valent vingt points à
//                       eux seuls -- alors il prend des mains qu'il faut
//                       laisser et laisse des mains qu'il fallait prendre. À
//                       partir d'intermédiaire la main est pesée : la longueur
//                       d'atout, le valet, la belote, les as dehors, et les
//                       couleurs vides qui feront des coupes.
//
//   enchérir           À la coinche, la même lecture sert à autre chose : il
//                       ne s'agit plus de prendre ou non, mais de dire un
//                       chiffre qu'on devra tenir. Un débutant annonce ce
//                       qu'il voudrait faire ; les autres annoncent ce qu'ils
//                       pensent faire, et laissent le contrat à l'adversaire
//                       quand il monte au-dessus de leur main. Coincher, c'est
//                       la même question posée à l'envers : est-ce que ce qui
//                       me reste suffit à l'empêcher d'y arriver ?
//
//   le partenaire       C'est la vraie frontière entre le débutant et les
//                       autres. Quand le pli est au partenaire, il faut le
//                       charger -- y mettre un dix, un as -- et quand il ne
//                       l'est pas encore, il faut attendre. Un débutant joue
//                       sa carte comme s'il était seul à table, et c'est ce
//                       qui se voit le plus : il met son as sous l'as de son
//                       propre camp, et laisse filer les plis qu'il tenait.
//
//   compter les cartes  Un expert sait ce qui est sorti, donc ce qui reste
//                       dehors, donc si son roi est devenu maître et si son
//                       dix peut encore passer. Les autres jouent avec ce
//                       qu'ils voient sur la table, ce qui n'est pas la même
//                       chose du tout : un roi dont l'as est tombé reste une
//                       carte qu'ils n'osent pas jouer.
//
//   le dix de der       Garder un atout pour le dernier pli valant dix points,
//                       un expert ne se défausse pas d'un atout quand autre
//                       chose coûte le même prix.
//
//   la bourde           Et par-dessus tout cela, un débutant joue franchement
//                       la mauvaise carte de temps en temps. Un coup sur trois
//                       tout en bas, aucun tout en haut.

struct Skill {
    qreal mastery = 0.0;
    qreal blunder = 0.0;         // la mauvaise carte, exprès
    bool readsPartner = false;   // sait qu'on joue à deux
    bool countsCards = false;    // se souvient de tout ce qui est passé
    bool weighsHand = false;     // pèse la main avant de prendre
    bool savesDer = false;       // garde un atout pour le dernier pli
    qreal boldness = 1.0;        // avec quelle facilité il prend
};

Skill skillFor(int xp);
Skill bestSkill();               // celui avec lequel le bouton Conseil pense

// Ce que la main vaut, en points qu'on espère faire, si l'atout est `trump`.
// La retourne est comprise dedans quand elle revient au preneur : au moment de
// l'enchère on a cinq cartes et on en aura huit, et une estimation sur cinq
// cartes qui ignore la sixième qu'on va ramasser n'estime pas la bonne main.
//
// Sortie d'ici plutôt que cachée dans le cerveau, parce que le bouton Conseil
// en dit le chiffre : savoir qu'une main vaut quarante points à trèfle et
// soixante à cœur est à peu près tout ce qu'il y a à apprendre de l'enchère.
int handEstimate(const QVector<Card> &hand, Suit trump);

// Ce qu'il faut, à peu près, pour que prendre soit raisonnable. Le camp doit
// faire quatre-vingt-deux points, le partenaire en apporte une vingtaine sans
// qu'on sache lesquels, et les deux cartes qu'on recevra encore une dizaine :
// le reste est à nous.
//
// Le chiffre a été réglé en jouant : c'est à peu près le soixante-quinzième
// centile des mains, ce qui fait qu'un joueur sur quatre prend à chaque tour
// de parole -- et donc qu'une donne sur dix environ n'est prise par personne,
// ce qui est à peu près ce qu'on voit à une vraie table. Plus haut, les quatre
// passent trop souvent et on passe la soirée à redonner ; plus bas, on prend
// des mains qui tombent.
inline constexpr int kTakeEstimate = 40;

// Les deux droites de l'enchère de la coinche, mesurées en jouant.
//
// Prendre et défendre ne sont pas la même place, et il faut donc deux
// chiffres. Du côté du preneur : son camp ramasse soixante-douze points plus
// un peu moins des deux tiers de ce que sa main vaut. Du côté du défenseur :
// le preneur ramasse cent vingt-trois points moins les trois quarts de ce que
// le défenseur tient dans la couleur annoncée.
//
// Les deux droites ont été mesurées sur de vraies enchères, et il a fallu s'y
// reprendre : mesurées sur un preneur tiré au sort, elles disaient que le
// contrat tombait bien plus souvent qu'il ne tombe. Un preneur n'est pas tiré
// au sort -- il s'est choisi, et c'est pour ça qu'il réussit.
inline constexpr int kBaselineTricks = 72;
inline constexpr int kTakerUnopposed = 123;

// La marge qu'on garde en annonçant : on ne dit pas ce qu'on fera en moyenne,
// parce que la moyenne se rate une fois sur deux.
inline constexpr int kBidMargin = 14;

// Et de combien il faut être sûr avant de coincher.
//
// Mesuré comme le reste, et le résultat est honnête à dire : avec cette marge
// les adversaires coinchent deux donnes sur cent, et le contrat tombe un peu
// moins d'une fois sur deux. Monter la marge n'améliore pas le taux, et le
// réserver aux gros contrats non plus -- on a essayé les deux. Ce n'est pas un
// défaut du calcul : un défenseur ne voit que sa main, et ce que fera celle de
// son partenaire est précisément ce qui manque. Coincher est un pari, à une
// vraie table aussi.
inline constexpr int kCoincheMargin = 18;

struct Bid {
    bool take = false;       // à la retourne : prendre ; à la coinche : annoncer
    bool coinche = false;    // à la coinche : coincher, ou surcoincher
    Suit suit = Suit::Pique;
    int value = 0;           // à la coinche : le contrat annoncé
};

// Ce qu'on dit, maintenant, à cette place. Appelé une fois par tour de parole,
// et il répond pour les deux façons d'enchérir.
Bid chooseBid(const Belote &game, int at, const Skill &skill);

// La carte à jouer, maintenant. Appelée une fois par pli et par joueur.
int chooseCard(const Belote &game, int at, const Skill &skill);

// Ce que le meilleur joueur de la table ferait de votre main, dit en français.
// C'est le bouton Conseil, et c'est le même cerveau : il n'y en a pas un
// second, meilleur, gardé pour le joueur.
QString advise(const Belote &game, int at);
