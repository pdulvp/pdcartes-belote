#pragma once

#include "Belote.h"
#include "Brain.h"
#include "Tables.h"

#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;
class QPushButton;
class QTimer;
class ScoreBoard;
class TableView;
class VictoryEffect;

// Tout ce qu'on peut régler, et ce n'est pas grand-chose : une belote demande
// une table, et deux ou trois commodités.
struct GameSettings {
    QString tableId;
    // Comment la parole est distribuée : à la retourne, ou à la coinche. Et si
    // les tierces, cinquantes et carrés comptent. Ces deux-là sont des règles
    // et non des commodités : elles ne changent pas une partie en cours.
    Variant variant = Variant::Classique;
    bool annonces = true;
    // Dans quel ordre la main est tenue une fois l'atout connu. Par rang, qui
    // est l'ordre où l'on range des cartes ; ou par force, qui est l'ordre où
    // l'on s'en sert -- le dix passe alors devant le roi, et le valet d'atout
    // devant tout. Les deux boutons sont là dans les deux cas : ceci ne décide
    // que de celui qui s'applique tout seul au début de chaque donne.
    bool holdByStrength = false;
    bool offerHints = true;     // le bouton Conseil, et le compte de la main
    bool celebrate = true;      // le feu d'artifice à la fin
    int pace = 1;               // 0 posé, 1 normal, 2 vif : le temps de réflexion
    // La taille des cartes, en pour cent. Un jeu installé apporte des images
    // d'une finesse donnée, et une figure réduite au quart de cela n'est plus
    // qu'un bruit de couleurs : voir TableView::setZoom().
    int zoom = 100;
    // La couleur du drap, et de tout ce qui va avec. Elle n'appartient pas à
    // la table mais au profil, qui garde la sienne : voir TableLook.h.
    int hue = 0;
    QVector<QString> invited;   // les adversaires demandés, les autres au tour de rôle
};

// La page de jeu : la table, la rangée de boutons sous elle, et ce qui fait
// jouer les trois autres.
//
// C'est ici que les deux moitiés du programme se rencontrent. En dessous, les
// règles, qui ne savent rien de l'écran ; à côté, les cerveaux, qui ne savent
// rien du temps qui passe. Cette page-là tient la partie, demande un coup à
// celui dont c'est le tour, l'attend un instant pour qu'on le voie arriver,
// laisse le pli sur la table le temps qu'on le lise, et compte l'expérience à
// la fin de chaque donne.
class TablePage : public QWidget {
    Q_OBJECT

public:
    explicit TablePage(QWidget *parent = nullptr);

    void startMatch(const GameSettings &settings);
    void applySettings(const GameSettings &settings);
    void refresh();

    const Belote *game() const { return m_game; }
    bool playing() const;
    QString subtitle() const;

Q_SIGNALS:
    void stateChanged();
    // Demandé par la feuille de marque : on quitte la table.
    void wantsMenu();
    // Changée à la molette sur le drap : la fenêtre la garde.
    void zoomChanged(int percent);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QWidget *buildActions();

    void onTurnBegan(int seat);
    void onActed(int seat, const QString &tale);
    void onTrickFinished(int winner);
    void onDonneFinished();
    void onMatchFinished();
    void playRival();
    void sweep();

    // Les cartes qui sont dans les quatre mains, dans l'ordre où elles ont été
    // données. Relevées avant la prise, parce que la prise complète les quatre
    // mains d'un coup et qu'on ne saurait plus ensuite lesquelles sont neuves.
    QVector<int> cardsInHands() const;
    // Ce que la prise vient d'ajouter, montré en l'air : la retourne part de
    // sa place au milieu, le reste du talon.
    void noteTakeDeal(const QVector<int> &before, int retourneId);
    // La donne qui vient d'être distribuée, montrée une fois et une seule.
    void showDealIfNew();

    void sort(bool byStrength);
    void stepBid(int by);
    // L'ordre choisi dans les réglages, posé au moment où l'atout est connu --
    // le seul moment où il y ait quelque chose à ranger, puisqu'une main de
    // belote ne fait ensuite que diminuer.
    void sortAsPreferred();
    void doTake(Suit suit);
    void doSuit(Suit suit);      // prendre, ou annoncer : selon la façon de jouer
    void doBid(int value, Suit suit);
    void doCoinche();
    void doPass();
    void doPlay();
    void doHint();
    void onCardPressed(int cardId);
    void onCardDoubleClicked(int cardId);
    void moveChoice(int step);

    void updateActions();
    // La bande du bas porte onze boutons à la coinche : c'est la phrase qui
    // cède, et elle cède proprement plutôt qu'en se faisant couper au milieu
    // d'un mot.
    void setState(const QString &text);
    void elideState();
    void sayWhatToDo();
    void grumble(const QString &why);
    int mySeat() const;
    int thinkingMs(bool first) const;
    int sweepMs() const;
    QVector<SeatSpec> chooseSeats() const;

    Belote *m_game = nullptr;
    TableView *m_view = nullptr;
    ScoreBoard *m_board = nullptr;
    VictoryEffect *m_victory = nullptr;
    QWidget *m_actions = nullptr;

    QPushButton *m_sortSuit = nullptr;
    QPushButton *m_sortStrength = nullptr;
    QPushButton *m_takeSuit[4] = {nullptr, nullptr, nullptr, nullptr};
    // L'enchère de la coinche : le contrat qu'on s'apprête à dire, et de quoi
    // le faire monter ou descendre.
    QPushButton *m_less = nullptr;
    QPushButton *m_more = nullptr;
    QPushButton *m_capot = nullptr;
    QPushButton *m_coinche = nullptr;
    QPushButton *m_pass = nullptr;
    QPushButton *m_play = nullptr;
    QPushButton *m_hint = nullptr;
    QLabel *m_state = nullptr;

    QString m_stateText;
    GameSettings m_settings;
    QVector<Skill> m_skills;
    QTimer *m_beat = nullptr;
    QTimer *m_sweep = nullptr;
    int m_bidValue = kLowestBid;
    // La donne dont la distribution a déjà été montrée. Il n'y a pas de signal
    // « on vient de donner » : c'est le numéro de la donne qui le dit, et le
    // comparer est ce qui évite de redistribuer à l'écran à chaque parole.
    int m_dealShown = 0;
    bool m_sortedDonne = false;
    bool m_donneCounted = false;
};
