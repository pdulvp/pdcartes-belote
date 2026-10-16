#pragma once

#include <QDialog>

class QTextBrowser;

// Les règles, et comment on joue à la souris.
//
// La belote ne se devine pas en cliquant. Le valet d'atout vaut vingt points
// et le neuf quatorze, mais seulement à l'atout ; on doit fournir à la couleur
// demandée, couper quand on ne l'a pas, monter sur un atout déjà posé, et rien
// de tout cela quand c'est le partenaire qui tient le pli ; le camp qui a pris
// doit faire quatre-vingt-deux points et n'en marque aucun s'il en fait
// quatre-vingt-un. Rien de tout cela ne se lit sur le tapis. Alors c'est
// écrit, en entier, dans la fenêtre plutôt que dans un fichier que personne ne
// trouvera.
class HelpDialog : public QDialog {
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = nullptr);

private:
    QTextBrowser *m_pages = nullptr;
};
