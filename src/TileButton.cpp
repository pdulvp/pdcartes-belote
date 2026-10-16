#include "TileButton.h"

#include "CardArt.h"
#include "Palette.h"

#include <QEnterEvent>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kCorner = 14.0;
constexpr int kArtHeight = 86;
constexpr int kPadding = 14;

// La petite table: un ovale de drap, quatre places autour, et la retourne au
// milieu. Ce n'est pas la table qu'on va jouer -- c'est son portrait, et ce
// qu'il dit, c'est qui joue avec qui.
void paintLittleTable(QPainter *painter, const QRectF &box, const Palette &scheme) {
    const QRectF cloth = box.adjusted(box.width() * 0.16, 4.0, -box.width() * 0.16, -4.0);
    if (cloth.width() < 20 || cloth.height() < 20) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(Qt::NoPen);
    painter->setBrush(blendColours(scheme.felt, scheme.feltEdge, 0.3));
    painter->drawEllipse(cloth.translated(0, 2.0));
    painter->setBrush(scheme.felt);
    painter->drawEllipse(cloth);
    painter->setBrush(Qt::NoBrush);
    QColor braid = scheme.trim;
    braid.setAlpha(170);
    painter->setPen(QPen(braid, 1.6));
    painter->drawEllipse(cloth.adjusted(3, 3, -3, -3));

    // Le talon au milieu, et la retourne posée en travers dessus : c'est elle
    // qu'on propose comme atout, et c'est ce qui fait qu'un ovale de drap est
    // une table de belote plutôt qu'une table de n'importe quoi.
    const QSizeF middle = cardSizeForHeight(cloth.height() * 0.30);
    QRectF stack(0, 0, middle.width(), middle.height());
    stack.moveCenter(cloth.center());
    for (int i = 2; i >= 1; --i) {
        drawCardBack(painter, stack.translated(-1.2 * i, -1.2 * i), scheme);
    }
    painter->save();
    painter->translate(stack.center());
    painter->rotate(74.0);
    painter->translate(-stack.center());
    drawCardBack(painter, stack, scheme);
    painter->restore();

    // Les quatre places, la vôtre en bas. Les deux d'un camp sont dans la
    // couleur de la table et les deux autres de dos : on joue avec celui d'en
    // face, et c'est la première chose que la tuile a à dire.
    const QSizeF seat = cardSizeForHeight(cloth.height() * 0.26);
    for (int i = 0; i < 4; ++i) {
        const qreal angle = M_PI / 2.0 + i * M_PI / 2.0;
        const QPointF at(cloth.center().x() + std::cos(angle) * cloth.width() * 0.40,
                         cloth.center().y() + std::sin(angle) * cloth.height() * 0.42);
        QRectF place(0, 0, seat.width(), seat.height());
        place.moveCenter(at);
        painter->save();
        painter->translate(at);
        painter->rotate(-angle * 180.0 / M_PI - 90.0);
        painter->translate(-at);
        if (i % 2 == 0) {
            painter->setPen(Qt::NoPen);
            // La vôtre pleine, celle d'en face un peu plus pâle : c'est le
            // même camp, et ce n'est pas la même personne.
            painter->setBrush(i == 0 ? scheme.accent
                                     : blendColours(scheme.accent, scheme.felt, 0.38));
            painter->drawRoundedRect(place, 3, 3);
        } else {
            drawCardBack(painter, place, scheme);
        }
        painter->restore();
    }
    painter->restore();
}

} // namespace

TileButton::TileButton(QWidget *parent) : QAbstractButton(parent) {
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);
}

void TileButton::setBlurb(const QString &blurb) {
    m_blurb = blurb;
    update();
}

void TileButton::setCorner(const QString &corner) {
    m_corner = corner;
    update();
}

void TileButton::setTileSize(const QSize &size) {
    m_tileSize = size;
    setMinimumSize(size);
    updateGeometry();
}

QSize TileButton::sizeHint() const { return m_tileSize; }

void TileButton::enterEvent(QEnterEvent *event) {
    m_hovered = true;
    update();
    Q_EMIT pointerIn();
    QAbstractButton::enterEvent(event);
}

void TileButton::leaveEvent(QEvent *event) {
    m_hovered = false;
    update();
    Q_EMIT pointerOut();
    QAbstractButton::leaveEvent(event);
}

void TileButton::paintEvent(QPaintEvent *) {
    const Palette &scheme = currentPalette();
    const bool lit = m_hovered || hasFocus();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QRectF box = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    if (isDown()) box.translate(0, 1);

    const QColor fill = isChecked() ? blendColours(scheme.surface, scheme.accent, 0.20)
                                    : (lit ? blendColours(scheme.surface, scheme.accent, 0.10)
                                           : scheme.surface);

    QPainterPath shape;
    shape.addRoundedRect(box, kCorner, kCorner);

    if (lit || isChecked()) {
        QPainterPath lift;
        lift.addRoundedRect(box.translated(0, 2.5), kCorner, kCorner);
        painter.fillPath(lift, blendColours(scheme.desk, scheme.accent, 0.28));
    }

    painter.fillPath(shape, fill);
    painter.setPen(QPen(isChecked() || lit ? scheme.accent : scheme.edge,
                        isChecked() ? 2.2 : (lit ? 2.0 : 1.2)));
    painter.drawPath(shape);

    const QRectF inner = box.adjusted(kPadding, kPadding, -kPadding, -kPadding);
    const QRectF art(inner.left(), inner.top(), inner.width(),
                     std::min(qreal(kArtHeight), inner.height() * 0.52));
    paintLittleTable(&painter, art, scheme);

    QFont name = font();
    name.setBold(true);
    if (name.pointSizeF() > 0) name.setPointSizeF(name.pointSizeF() * 1.08);
    painter.setFont(name);
    painter.setPen(scheme.surfaceText);
    const QFontMetricsF nameMetrics(name);
    const QRectF nameBox(inner.left(), art.bottom() + 6, inner.width(), nameMetrics.height());
    painter.drawText(nameBox, Qt::AlignHCenter | Qt::AlignVCenter, text());

    if (!m_blurb.isEmpty()) {
        QFont aside = font();
        if (aside.pointSizeF() > 0) aside.setPointSizeF(std::max(7.0, aside.pointSizeF() - 1.4));
        painter.setFont(aside);
        painter.setPen(scheme.hint);
        const QRectF blurbBox(inner.left(), nameBox.bottom() + 2, inner.width(),
                              inner.bottom() - nameBox.bottom() - 2);
        painter.drawText(blurbBox, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_blurb);
    }

    if (!m_corner.isEmpty()) {
        QFont badge = font();
        badge.setBold(true);
        if (badge.pointSizeF() > 0) badge.setPointSizeF(std::max(7.0, badge.pointSizeF() - 1.6));
        painter.setFont(badge);
        const QFontMetricsF metrics(badge);
        QRectF chip(0, 0, metrics.horizontalAdvance(m_corner) + 16.0, metrics.height() + 4.0);
        chip.moveTopRight(QPointF(box.right() - 9.0, box.top() + 9.0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(scheme.accent);
        painter.drawRoundedRect(chip, chip.height() / 2.0, chip.height() / 2.0);
        painter.setPen(scheme.accentText);
        painter.drawText(chip, Qt::AlignCenter, m_corner);
    }
}
