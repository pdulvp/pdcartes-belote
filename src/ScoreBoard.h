#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QPushButton;
class Belote;

// La feuille de marque, posée sur le tapis à la fin d'une donne.
//
// Elle couvre le drap au lieu de le remplacer : les plaques et l'atout sont
// encore là dessous, ce qui est ce qu'on veut regarder pendant qu'on lit ce
// que la donne a coûté.
//
// Deux formes, la même carte : à la fin d'une donne c'est ce que celle-ci a
// donné et où en sont les deux camps ; à la fin de la partie elle dit qui l'a
// gagnée. Et elle écrit les deux chiffres qui ne sont pas le même -- ce que
// chaque camp a ramassé dans les plis, et ce qu'il a marqué -- parce qu'un
// camp dedans ramasse soixante-dix points et en marque zéro, et que c'est
// précisément la chose qu'il faut comprendre de la belote.
class ScoreBoard : public QWidget {
    Q_OBJECT

public:
    explicit ScoreBoard(QWidget *parent = nullptr);

    void showDonne(const Belote *game);
    void showMatch(const Belote *game);
    void hideBoard();

    void refresh();

Q_SIGNALS:
    // La donne suivante, ou une nouvelle partie : selon ce que la feuille
    // montre.
    void goOn();
    void leave();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    struct Line {
        QString name;
        int hue = 0;
        int tricks = 0;
        int points = 0;      // ce qu'il a ramassé dans les plis
        int gained = 0;      // ce qu'il a marqué
        int total = 0;
        bool ours = false;
        bool winner = false;
    };

    QSizeF measure() const;
    void placeButtons();
    void dressButtons();

    QVector<Line> m_lines;
    QString m_headline;
    QString m_aside;
    QStringList m_notes;
    bool m_matchOver = false;
    bool m_youWon = false;

    QPushButton *m_onwards = nullptr;
    QPushButton *m_away = nullptr;
};
