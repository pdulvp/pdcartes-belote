#pragma once

#include <QPoint>
#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;
class QPushButton;
class RivalCard;
class TileButton;

// Le menu d'ouverture : les cinq tables, et les six joueurs.
//
// Il n'y a ni bouton « nouvelle partie » ni boîte de dialogue à traverser :
// presser une table est ce qui lance la partie, parce que choisir jusqu'où
// elle va est le seul choix qu'une nouvelle partie demande. Tout le reste de
// la page répond à la question d'avant, qui à la belote n'est pas « contre
// qui » mais « avec qui » : le premier invité s'assoit en face de vous.
//
// Le profil n'est plus ici : il se tient dans la bande du haut, où il reste
// visible pendant la partie. Et ce qui se fait d'un joueur se fait sur lui,
// d'un clic droit, plutôt que dans les réglages.
class TablePicker : public QWidget {
    Q_OBJECT

public:
    explicit TablePicker(QWidget *parent = nullptr);

    // Qui a été invité, dans l'ordre. Le premier est votre partenaire ; les
    // places qui restent vont aux moins joués.
    QVector<QString> invited() const;

    // Après un changement de profil, ou quand l'expérience a bougé.
    void reread();
    void refresh();

Q_SIGNALS:
    void tableChosen(const QString &id);

private:
    void showRivalMenu(RivalCard *card, const QPoint &where);
    void renameRival(const QString &rivalId);
    void askResetRival(const QString &rivalId);

    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    QLabel *m_rivalNote = nullptr;
    QLabel *m_footer = nullptr;
    QVector<TileButton *> m_tiles;
    QVector<RivalCard *> m_cards;
    // L'ordre dans lequel on les a invités, parce que le premier ne s'assoit
    // pas à la même place que le deuxième.
    QVector<QString> m_order;
};
