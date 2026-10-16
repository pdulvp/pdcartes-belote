#include "Annonces.h"

#include <QCoreApplication>

#include <algorithm>

namespace {

QString sequenceName(int length) {
    switch (length) {
    case 3: return QCoreApplication::translate("annonces", "une tierce");
    case 4: return QCoreApplication::translate("annonces", "une cinquante");
    default: return QCoreApplication::translate("annonces", "un cent");
    }
}

QString carreName(int rank) {
    switch (rank) {
    case kValet: return QCoreApplication::translate("annonces", "un carré de valets");
    case kNeuf: return QCoreApplication::translate("annonces", "un carré de neufs");
    case kAs: return QCoreApplication::translate("annonces", "un carré d'as");
    case kDix: return QCoreApplication::translate("annonces", "un carré de dix");
    case kRoi: return QCoreApplication::translate("annonces", "un carré de rois");
    default: return QCoreApplication::translate("annonces", "un carré de dames");
    }
}

int sequenceValue(int length) {
    if (length >= 5) return kCent;
    if (length == 4) return kCinquante;
    return kTierce;
}

int carreValue(int rank) {
    switch (rank) {
    case kValet: return kCarreValets;
    case kNeuf: return kCarreNeufs;
    case kAs:
    case kDix:
    case kRoi:
    case kDame: return kCarreOrdinaire;
    default: return 0;      // les huit et les sept
    }
}

} // namespace

QString Annonce::name() const {
    if (!valid()) return QString();
    if (kind == AnnonceKind::Carre) return carreName(topRank);
    // La hauteur fait partie du nom, parce que c'est elle qui départage deux
    // annonces de même longueur : « une tierce au roi » se compare, « une
    // tierce » ne se compare pas.
    return QCoreApplication::translate("annonces", "%1 %2 au %3")
        .arg(sequenceName(length), suitWithArticle(suit), rankLabel(topRank));
}

QVector<Annonce> annoncesIn(const QVector<Card> &hand) {
    QVector<Annonce> found;

    // Les suites, couleur par couleur. Une couleur peut en porter deux -- un
    // 7-8-9 et un D-R-A tiennent dans la même main et comptent tous les deux.
    for (int s = 0; s < 4; ++s) {
        const Suit suit = Suit(s);
        QVector<Card> run;
        for (int rank = kSept; rank <= kAs; ++rank) {
            const Card *held = nullptr;
            for (const Card &card : hand) {
                if (card.suit == suit && card.rank == rank) held = &card;
            }
            if (held) {
                run.append(*held);
                if (rank != kAs) continue;
            }
            if (run.size() >= 3) {
                Annonce annonce;
                annonce.kind = AnnonceKind::Sequence;
                annonce.length = run.size();
                annonce.points = sequenceValue(run.size());
                annonce.topRank = run.last().rank;
                annonce.suit = suit;
                annonce.cards = run;
                found.append(annonce);
            }
            run.clear();
        }
    }

    // Les carrés. La carte qui est dans un carré peut être aussi dans une
    // suite : elle est comptée des deux côtés, et c'est la règle.
    for (int rank = kSept; rank <= kAs; ++rank) {
        const int value = carreValue(rank);
        if (value <= 0) continue;
        QVector<Card> four;
        for (const Card &card : hand) {
            if (card.rank == rank) four.append(card);
        }
        if (four.size() < 4) continue;
        Annonce annonce;
        annonce.kind = AnnonceKind::Carre;
        annonce.points = value;
        annonce.topRank = rank;
        annonce.cards = four;
        found.append(annonce);
    }

    return found;
}

int annonceTotal(const QVector<Annonce> &list) {
    int total = 0;
    for (const Annonce &annonce : list) total += annonce.points;
    return total;
}

int compareAnnonces(const Annonce &a, const Annonce &b, Suit trump, bool hasTrump) {
    if (!a.valid() && !b.valid()) return 0;
    if (!a.valid()) return -1;
    if (!b.valid()) return 1;

    // Un carré passe devant n'importe quelle suite, même devant un cent qui
    // vaut autant que lui : quatre valets sont quatre valets.
    if (a.kind != b.kind) return a.kind == AnnonceKind::Carre ? 1 : -1;

    if (a.kind == AnnonceKind::Carre) {
        if (a.points != b.points) return a.points > b.points ? 1 : -1;
        // Les quatre carrés à cent points se départagent dans l'ordre
        // ordinaire des cartes : l'as, le dix, le roi, la dame.
        Card left;
        left.rank = a.topRank;
        Card right;
        right.rank = b.topRank;
        const int mine = cardStrength(left, trump, false);
        const int theirs = cardStrength(right, trump, false);
        if (mine != theirs) return mine > theirs ? 1 : -1;
        return 0;
    }

    if (a.length != b.length) return a.length > b.length ? 1 : -1;
    if (a.topRank != b.topRank) return a.topRank > b.topRank ? 1 : -1;
    // Même longueur et même hauteur : l'atout tranche, et à défaut d'atout
    // les deux s'annulent et personne ne marque.
    if (hasTrump && a.suit != b.suit) {
        if (a.suit == trump) return 1;
        if (b.suit == trump) return -1;
    }
    return 0;
}

Annonce bestAnnonce(const QVector<Annonce> &list, Suit trump, bool hasTrump) {
    Annonce best;
    for (const Annonce &annonce : list) {
        if (!best.valid() || compareAnnonces(annonce, best, trump, hasTrump) > 0) best = annonce;
    }
    return best;
}
