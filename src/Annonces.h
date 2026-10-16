#pragma once

#include "Cards.h"

#include <QString>
#include <QVector>

// Les annonces : ce qu'une main déclare avant de la jouer.
//
// Au premier pli, chacun annonce ce qu'il a de suites et de carrés. Ce n'est
// pas un supplément de décor : une cinquante vaut cinquante points, un carré
// de valets deux cents, et deux cents points sont plus que la donne entière.
// Une main sans un pli peut valoir plus qu'une main qui les fait tous.
//
// Trois choses à savoir, et elles sont toutes les trois contre-intuitives :
//
//   l'ordre n'est pas celui de l'atout   Une suite se lit dans l'ordre
//        naturel -- 7, 8, 9, 10, valet, dame, roi, as -- et non dans celui qui
//        prend les plis. Le valet d'atout, qui passe devant tout le reste,
//        n'est ici qu'entre le dix et la dame.
//
//   un seul camp marque   Le camp qui tient la plus forte annonce marque
//        toutes les siennes, et l'autre n'en marque aucune. Une cinquante
//        contre trois tierces, et les trois tierces ne valent rien.
//
//   elles ne comptent pas dans le contrat   Le contrat se joue sur les cent
//        soixante-deux points des plis. Les annonces s'ajoutent après -- et le
//        camp preneur qui chute perd les siennes.
//
// La belote et rebelote, elle, n'est pas une annonce : elle ne se compare à
// rien, et elle est acquise quoi qu'il arrive. Elle vit donc avec les règles,
// et non ici.

enum class AnnonceKind { Sequence, Carre };

// Ce que valent les annonces.
inline constexpr int kTierce = 20;          // trois cartes qui se suivent
inline constexpr int kCinquante = 50;       // quatre
inline constexpr int kCent = 100;           // cinq et au-delà
inline constexpr int kCarreValets = 200;
inline constexpr int kCarreNeufs = 150;
inline constexpr int kCarreOrdinaire = 100; // as, dix, rois, dames
// Les carrés de huit et de sept ne valent rien, ce qui est la seule chose que
// ces deux cartes aient en commun avec le reste de leur vie.

struct Annonce {
    AnnonceKind kind = AnnonceKind::Sequence;
    int points = 0;
    int length = 0;             // la suite : trois, quatre, cinq ou plus
    int topRank = kSept;        // la suite : la plus haute ; le carré : le rang
    Suit suit = Suit::Pique;    // la suite seulement
    QVector<Card> cards;

    bool valid() const { return points > 0; }
    // « une tierce à cœur au roi », « un carré de valets ».
    QString name() const;
};

// Tout ce que cette main déclare : la plus longue suite de chaque série de
// cartes qui se suivent, et les carrés. Une même carte peut servir dans les
// deux -- le valet d'un carré de valets est aussi dans la tierce où il tombe.
QVector<Annonce> annoncesIn(const QVector<Card> &hand);

int annonceTotal(const QVector<Annonce> &list);

// Laquelle des deux l'emporte : positif pour `a`, négatif pour `b`, zéro
// quand elles s'annulent -- deux tierces de même hauteur dont aucune n'est à
// l'atout, et personne ne marque.
int compareAnnonces(const Annonce &a, const Annonce &b, Suit trump, bool hasTrump = true);

// La plus forte de la liste, ou une annonce vide si la liste l'est.
Annonce bestAnnonce(const QVector<Annonce> &list, Suit trump, bool hasTrump = true);
