#include "Tables.h"

#include <QCoreApplication>

#include <algorithm>

namespace {

const QVector<TableKind> &builtTables() {
    static const QVector<TableKind> kinds = {
        {QStringLiteral("donne"), QCoreApplication::translate("tables", "Une donne"),
         QCoreApplication::translate("tables", "Une seule, et la meilleure marque l'emporte"), 0},
        {QStringLiteral("comptoir"), QCoreApplication::translate("tables", "Le comptoir"),
         QCoreApplication::translate("tables", "Courte : une prise ratée et c'est fini"), 301},
        {QStringLiteral("tapis"), QCoreApplication::translate("tables", "Le tapis vert"),
         QCoreApplication::translate("tables", "La belote telle qu'on la joue"), 501},
        {QStringLiteral("cercle"), QCoreApplication::translate("tables", "Le cercle"),
         QCoreApplication::translate("tables", "De quoi se refaire après un capot"), 701},
        {QStringLiteral("soiree"), QCoreApplication::translate("tables", "La soirée"),
         QCoreApplication::translate("tables", "Assez longue pour que les six y apprennent"),
         1000},
    };
    return kinds;
}

} // namespace

const QVector<TableKind> &tableKinds() { return builtTables(); }

const TableKind &tableKindById(const QString &id) {
    for (const TableKind &kind : builtTables()) {
        if (kind.id == id) return kind;
    }
    // La table par défaut, pour qu'un fichier de réglages écrit par une
    // version plus tardive ne laisse pas le jeu sans table du tout.
    return builtTables().at(2);
}

QString defaultTableId() { return builtTables().at(2).id; }

int donnesFor(int points) {
    if (points <= 0) return 0;
    return std::max(1, (points + 45) / 90);
}
