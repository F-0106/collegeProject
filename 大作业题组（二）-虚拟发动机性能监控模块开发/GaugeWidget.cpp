#include "GaugeWidget.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <cmath>

namespace eicas {

GaugeWidget::GaugeWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(210, 170);
}

void GaugeWidget::setTitle(const QString& title) {
    m_title = title;
    update();
}

void GaugeWidget::setUnit(const QString& unit) {
    m_unit = unit;
    update();
}

void GaugeWidget::setRange(double minValue, double maxValue) {
    m_minValue = minValue;
    m_maxValue = maxValue;
    update();
}

void GaugeWidget::setValue(std::optional<double> value, ValueState state) {
    m_value = value;
    m_state = state;
    update();
}

void GaugeWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(0, 0, 0));

    const int side = std::min(width(), height() + 30);
    QRectF arcRect((width() - side) / 2.0, 16.0, side, side);
    arcRect.adjust(18, 18, -18, -18);

    const QPointF center = arcRect.center();
    const double radius = arcRect.width() / 2.0;
    const double startDeg = 205.0;
    const double spanDeg = -210.0;

    p.setPen(QPen(QColor(170, 176, 184), 2));
    p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect, static_cast<int>(startDeg * 16), static_cast<int>(spanDeg * 16));

    p.setPen(QPen(QColor(150, 150, 150), 1));
    for (int i = 0; i <= 5; ++i) {
        const double angle = (startDeg + spanDeg * i / 5.0) * 3.14159265358979323846 / 180.0;
        const QPointF outer(center.x() + std::cos(angle) * radius, center.y() - std::sin(angle) * radius);
        const QPointF inner(center.x() + std::cos(angle) * (radius - 12), center.y() - std::sin(angle) * (radius - 12));
        p.drawLine(inner, outer);
    }

    if (m_value.has_value()) {
        const double ratio = std::clamp((m_value.value() - m_minValue) / (m_maxValue - m_minValue), 0.0, 1.0);
        const double valueSpan = spanDeg * ratio;

        QPainterPath sector;
        sector.moveTo(center);
        sector.arcTo(arcRect, startDeg, valueSpan);
        sector.closeSubpath();
        p.setPen(Qt::NoPen);
        QColor fill = colorForState(m_state);
        fill.setAlpha(210);
        p.setBrush(fill);
        p.drawPath(sector);
    }

    p.setPen(QPen(QColor(90, 96, 110), 1));
    p.setBrush(Qt::NoBrush);
    QRectF valueBox(width() / 2.0 - 62.0, 18.0, 124.0, 42.0);
    p.drawRect(valueBox);

    QFont valueFont("Consolas", 24, QFont::Bold);
    p.setFont(valueFont);
    p.setPen(colorForState(m_state));
    p.drawText(valueBox, Qt::AlignCenter, valueText());

    QFont labelFont("Consolas", 18, QFont::Bold);
    p.setFont(labelFont);
    p.setPen(QColor(210, 210, 210));
    p.drawText(QRectF(0, height() - 46.0, width(), 28.0), Qt::AlignCenter, m_title);

    QFont unitFont("Consolas", 11);
    p.setFont(unitFont);
    p.setPen(QColor(130, 135, 145));
    p.drawText(QRectF(0, height() - 23.0, width(), 18.0), Qt::AlignCenter, m_unit);
}

QSize GaugeWidget::sizeHint() const {
    return QSize(240, 190);
}

QColor GaugeWidget::colorForState(ValueState state) const {
    switch (state) {
    case ValueState::Normal:
        return QColor(238, 238, 238);
    case ValueState::Caution:
        return QColor(255, 200, 35);
    case ValueState::Warning:
        return QColor(242, 35, 45);
    case ValueState::Invalid:
        return QColor(150, 150, 150);
    }
    return QColor(238, 238, 238);
}

QString GaugeWidget::valueText() const {
    if (!m_value.has_value() || m_state == ValueState::Invalid) {
        return "--";
    }
    if (m_maxValue > 500.0) {
        return QString::number(m_value.value(), 'f', 0);
    }
    return QString::number(m_value.value(), 'f', 1);
}

}  // namespace eicas
