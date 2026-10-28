#include "MenuStyle.h"

#include "Palette.h"

#include <QMenu>
#include <QString>

void dressMenu(QMenu *menu) {
    if (!menu) return;
    const Palette &scheme = currentPalette();
    menu->setStyleSheet(
        QStringLiteral("QMenu { background: %1; color: %2; border: 1px solid %3;"
                       " border-radius: 8px; padding: 5px; }"
                       // Un pixel de moins en haut qu'en bas, comme partout où Qt
                       // centre lui-même : voir `capCentred` dans Fonts.h.
                       "QMenu::item { padding: 5px 22px 7px 14px;"
                       " border-radius: 5px; }"
                       "QMenu::item:selected { background: %4; color: %5; }"
                       "QMenu::item:disabled { color: %6; }"
                       "QMenu::separator { height: 1px; background: %3; margin: 5px 8px; }")
            .arg(scheme.surface.name(), scheme.surfaceText.name(), scheme.edge.name(),
                 scheme.accent.name(), scheme.accentText.name(), scheme.hint.name()));
}
