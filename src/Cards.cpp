#include "Cards.h"

#include "CardArt.h"

#include <QCoreApplication>
#include <QRandomGenerator>

#include <algorithm>

namespace {

// La force, du sept à l'as, dans une couleur ordinaire : l'as devant le dix,
// puis le roi, la dame, le valet, et le reste dans l'ordre naturel.
int plainStrength(int rank) {
    switch (rank) {
    case kAs: return 8;
    case kDix: return 7;
    case kRoi: return 6;
    case kDame: return 5;
    case kValet: return 4;
    case kNeuf: return 3;
    case kHuit: return 2;
    default: return 1;      // le sept
    }
}

// Et à l'atout, où le valet et le neuf passent devant. C'est tout ce qui
// distingue la belote d'un jeu de levées ordinaire, et c'est assez pour que la
// même main ne vaille pas la même chose selon l'atout qu'on annonce.
int trumpStrength(int rank) {
    switch (rank) {
    case kValet: return 8;
    case kNeuf: return 7;
    case kAs: return 6;
    case kDix: return 5;
    case kRoi: return 4;
    case kDame: return 3;
    case kHuit: return 2;
    default: return 1;      // le sept
    }
}

} // namespace

QVector<Card> freshPack() {
    QVector<Card> pack;
    pack.reserve(kPackSize);
    int id = 0;
    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = kSept; rank <= kAs; ++rank) {
            Card card;
            card.id = id++;
            card.rank = rank;
            card.suit = Suit(suit);
            pack.append(card);
        }
    }
    return pack;
}

void shuffleCards(QVector<Card> &cards) {
    // Fisher-Yates, tiré du générateur de Qt plutôt que de std::shuffle : une
    // seule source de hasard dans le programme, et c'est celle d'où sortent la
    // donne et les hésitations des adversaires.
    for (int i = cards.size() - 1; i > 0; --i) {
        const int j = QRandomGenerator::global()->bounded(i + 1);
        std::swap(cards[i], cards[j]);
    }
}

// Les deux fonctions suivantes délèguent à la cartothèque, qui dessine les
// cartes et a donc les mêmes questions à résoudre : deux tables des rangs
// finiraient par diverger.
bool isRed(Suit suit) { return cardSuitIsRed(int(suit)); }

QString suitPip(Suit suit) {
    switch (suit) {
    case Suit::Pique: return QStringLiteral("♠");
    case Suit::Coeur: return QStringLiteral("♥");
    case Suit::Carreau: return QStringLiteral("♦");
    case Suit::Trefle: return QStringLiteral("♣");
    }
    return QString();
}

QString suitName(Suit suit) {
    switch (suit) {
    case Suit::Pique: return QCoreApplication::translate("cards", "pique");
    case Suit::Coeur: return QCoreApplication::translate("cards", "cœur");
    case Suit::Carreau: return QCoreApplication::translate("cards", "carreau");
    case Suit::Trefle: return QCoreApplication::translate("cards", "trèfle");
    }
    return QString();
}

QString suitWithArticle(Suit suit) {
    // « à pique », « à cœur » : c'est ainsi qu'on annonce un atout, et la
    // tournure ne change pas d'une couleur à l'autre.
    return QCoreApplication::translate("cards", "à %1").arg(suitName(suit));
}

// La cartothèque range l'as à 1, comme un jeu de cartes ordinaire ; la
// belote le compte en haut, au-dessus du roi. La traduction se fait ici,
// comme partout où un rang de belote sort du programme.
QString rankLabel(int rank) { return cardRankLabel(rank == kAs ? 1 : rank); }

QString cardLabel(const Card &card) {
    if (!card.valid()) return QString();
    return rankLabel(card.rank) + suitPip(card.suit);
}

QString cardName(const Card &card) {
    if (!card.valid()) return QString();

    QString head;
    switch (card.rank) {
    case kAs: head = QCoreApplication::translate("cards", "l'as"); break;
    case kRoi: head = QCoreApplication::translate("cards", "le roi"); break;
    case kDame: head = QCoreApplication::translate("cards", "la dame"); break;
    case kValet: head = QCoreApplication::translate("cards", "le valet"); break;
    default: head = QCoreApplication::translate("cards", "le %1").arg(card.rank); break;
    }
    return QCoreApplication::translate("cards", "%1 de %2").arg(head, suitName(card.suit));
}

int cardPoints(const Card &card, Suit trump, bool hasTrump) {
    if (!card.valid()) return 0;
    const bool atout = hasTrump && card.suit == trump;
    switch (card.rank) {
    case kValet: return atout ? 20 : 2;
    case kNeuf: return atout ? 14 : 0;
    case kAs: return 11;
    case kDix: return 10;
    case kRoi: return 4;
    case kDame: return 3;
    default: return 0;      // le huit et le sept ne valent rien, jamais
    }
}

int cardStrength(const Card &card, Suit trump, bool hasTrump) {
    if (!card.valid()) return 0;
    return (hasTrump && card.suit == trump) ? trumpStrength(card.rank) : plainStrength(card.rank);
}

bool beatsTrick(const Card &candidate, const Card &best, Suit led, Suit trump, bool hasTrump) {
    if (!candidate.valid()) return false;
    if (!best.valid()) return true;

    const bool mineTrump = hasTrump && candidate.suit == trump;
    const bool bestTrump = hasTrump && best.suit == trump;
    if (mineTrump != bestTrump) return mineTrump;          // l'atout coupe
    if (mineTrump) return cardStrength(candidate, trump, hasTrump) >
                          cardStrength(best, trump, hasTrump);

    // Hors atout, une carte qui n'est pas de la couleur demandée ne prend rien
    // -- c'est une défausse, et elle tombe sous la plus petite des cartes
    // fournies.
    if (candidate.suit != led) return false;
    if (best.suit != led) return true;
    return cardStrength(candidate, trump, hasTrump) > cardStrength(best, trump, hasTrump);
}

namespace {

// L'atout à gauche de la main, et les autres couleurs dans l'ordre habituel
// derrière : une main de belote se tient atout en premier, parce que c'est la
// couleur dont on compte les cartes pendant toute la donne.
int suitOrder(Suit suit, Suit trump, bool hasTrump) {
    if (hasTrump && suit == trump) return -1;
    return int(suit);
}

} // namespace

void sortBySuit(QVector<Card> &cards, Suit trump, bool hasTrump) {
    std::stable_sort(cards.begin(), cards.end(), [trump, hasTrump](const Card &a, const Card &b) {
        const int sa = suitOrder(a.suit, trump, hasTrump);
        const int sb = suitOrder(b.suit, trump, hasTrump);
        if (sa != sb) return sa < sb;
        return a.rank < b.rank;
    });
}

void sortByStrength(QVector<Card> &cards, Suit trump, bool hasTrump) {
    std::stable_sort(cards.begin(), cards.end(), [trump, hasTrump](const Card &a, const Card &b) {
        const int sa = suitOrder(a.suit, trump, hasTrump);
        const int sb = suitOrder(b.suit, trump, hasTrump);
        if (sa != sb) return sa < sb;
        // La plus forte en tête de sa couleur : c'est l'ordre dans lequel on
        // regarde ce qu'on a à jouer, et il n'est pas celui des rangs.
        return cardStrength(a, trump, hasTrump) > cardStrength(b, trump, hasTrump);
    });
}

Card takeCard(QVector<Card> &cards, int id) {
    for (int i = 0; i < cards.size(); ++i) {
        if (cards.at(i).id != id) continue;
        const Card found = cards.at(i);
        cards.remove(i);
        return found;
    }
    return Card{};
}

const Card *findCard(const QVector<Card> &cards, int id) {
    for (const Card &card : cards) {
        if (card.id == id) return &card;
    }
    return nullptr;
}

bool hasSuit(const QVector<Card> &cards, Suit suit) {
    for (const Card &card : cards) {
        if (card.suit == suit) return true;
    }
    return false;
}
