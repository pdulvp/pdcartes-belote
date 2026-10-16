#pragma once

#include <QString>
#include <QVector>

// Ce qu'est une carte, et ce qu'elle vaut.
//
// La belote se joue avec trente-deux cartes : du sept à l'as, dans les quatre
// couleurs, et pas une de plus. Il n'y a donc jamais deux fois la même carte
// sur la table -- ce qui change tout par rapport à un jeu qui en mélange deux :
// une carte jouée est une carte sortie pour de bon, et c'est ce qui rend le
// comptage possible. L'identité d'une carte est son rang et sa couleur, et
// `id` n'est là que pour que l'écran puisse montrer du doigt.
//
// Et une carte n'a pas une valeur mais deux, selon qu'elle est de l'atout ou
// non : le valet vaut deux points dans une couleur ordinaire et vingt à
// l'atout, le neuf rien du tout d'un côté et quatorze de l'autre. C'est la
// seule chose qu'il faut savoir de la belote avant de s'asseoir, et c'est
// pourquoi rien ici ne donne la valeur d'une carte sans qu'on dise l'atout.

enum class Suit { Pique = 0, Coeur = 1, Carreau = 2, Trefle = 3 };

// Les rangs, dans l'ordre où ils se suivent. L'as est en haut et le sept en
// bas : il n'y a pas de place pour l'as sous le sept, et la question ne se
// pose jamais.
inline constexpr int kSept = 7;
inline constexpr int kHuit = 8;
inline constexpr int kNeuf = 9;
inline constexpr int kDix = 10;
inline constexpr int kValet = 11;
inline constexpr int kDame = 12;
inline constexpr int kRoi = 13;
inline constexpr int kAs = 14;

struct Card {
    int id = -1;          // laquelle des trente-deux, pour l'écran
    int rank = kSept;
    Suit suit = Suit::Pique;

    bool valid() const { return id >= 0; }
};

// Les huit rangs d'une couleur, et les quatre couleurs à la suite : c'est
// ainsi que le jeu est rangé, et c'est ce qui permet de se souvenir de ce qui
// est passé avec un tableau de trente-deux cases plutôt qu'avec une liste.
inline constexpr int kSuitSize = kAs - kSept + 1;
inline int cardIdFor(int rank, Suit suit) {
    return int(suit) * kSuitSize + (rank - kSept);
}

inline bool operator==(const Card &a, const Card &b) { return a.id == b.id; }

// Le jeu : les trente-deux, dans l'ordre. Le mélange est le seul hasard qu'il
// y a dans une partie.
QVector<Card> freshPack();
void shuffleCards(QVector<Card> &cards);

inline constexpr int kPackSize = 32;
// Quatre joueurs, huit cartes chacun, huit plis : les quatre nombres sont le
// même fait vu de quatre côtés, et c'est pour cela qu'ils sont écrits
// ensemble. Le jeu est épuisé à la fin de la donne -- il n'y a ni pioche ni
// défausse, et tout ce qui est distribué est joué.
inline constexpr int kSeatCount = 4;
inline constexpr int kHandSize = 8;
inline constexpr int kTrickCount = 8;
// Et deux camps, parce qu'on joue avec celui d'en face : ce n'est pas un
// réglage, c'est la belote.
inline constexpr int kTeamCount = 2;

// Ce que vaut la donne, en points.
//
// Cent cinquante-deux dans les cartes, et dix de plus pour le dernier pli --
// le dix de der -- ce qui fait cent soixante-deux à se partager. Le camp qui a
// pris doit en faire plus de la moitié : quatre-vingt-deux. À quatre-vingt-un
// chacun, personne n'a gagné la donne, et c'est le litige.
inline constexpr int kCardPoints = 152;
inline constexpr int kDixDeDer = 10;
inline constexpr int kDonnePoints = kCardPoints + kDixDeDer;
inline constexpr int kContract = 82;
inline constexpr int kLitige = 81;
// Les huit plis : deux cent cinquante au lieu de cent soixante-deux.
inline constexpr int kCapot = 250;
// Le roi et la dame d'atout dans la même main.
inline constexpr int kBelotePoints = 20;

bool isRed(Suit suit);
QString suitPip(Suit suit);            // le symbole
QString suitName(Suit suit);           // « pique », pour une phrase
QString suitWithArticle(Suit suit);    // « à pique », « à cœur »
QString rankLabel(int rank);           // « 7 ».. « 10 », « V », « D », « R », « A »
QString cardLabel(const Card &card);   // « R♥ », pour une place étroite
QString cardName(const Card &card);    // « le roi de cœur », pour une phrase

// Ce que la carte rapporte à celui qui ramasse le pli. Deux barèmes, selon
// qu'elle est de l'atout ou non.
int cardPoints(const Card &card, Suit trump, bool hasTrump = true);

// La force d'une carte pour prendre un pli, à l'intérieur de sa couleur. Deux
// ordres là aussi, et ce n'est pas le même : à l'atout le valet passe devant
// le neuf, qui passe devant l'as.
int cardStrength(const Card &card, Suit trump, bool hasTrump = true);

// `candidate` prend-elle le pli à `best`, la couleur demandée étant `led` ?
// L'atout bat tout le reste, et une carte d'une couleur qui n'a été ni
// demandée ni l'atout ne prend rien du tout.
bool beatsTrick(const Card &candidate, const Card &best, Suit led, Suit trump,
                bool hasTrump = true);

// Ranger la main. Par couleur, l'atout en tête, parce que c'est ainsi qu'on
// tient une main de belote ; ou par force, l'atout d'abord et les maîtresses
// en avant dans chaque couleur.
void sortBySuit(QVector<Card> &cards, Suit trump, bool hasTrump = true);
void sortByStrength(QVector<Card> &cards, Suit trump, bool hasTrump = true);

Card takeCard(QVector<Card> &cards, int id);
const Card *findCard(const QVector<Card> &cards, int id);
bool hasSuit(const QVector<Card> &cards, Suit suit);
