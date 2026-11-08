#include "ComponentPalette.h"

#include "SchematicWidget.h"

#include <QEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QToolButton>

namespace {
// Icon size in device-independent pixels. Each icon shows the schematic cell region
// x 20..120, y 44..144, which holds every symbol, so all icons share one scale and
// their wires line up.
constexpr QSize kIconSize(40, 40);
constexpr double kScale = kIconSize.width() / 100.0;

struct Entry
{
    rf::Kind kind;
    QChar key;  // same letter as the keyboard shortcut for the type
    std::vector<rf::Connection> connections;
};

QString connectionLabel(rf::Connection connection)
{
    switch (connection) {
    case rf::Connection::Series: return QObject::tr("Series");
    case rf::Connection::Shunt: return QObject::tr("Shunt");
    case rf::Connection::ThroughLine: return QObject::tr("Through");
    case rf::Connection::OpenStub: return QObject::tr("Open");
    case rf::Connection::ShortStub: return QObject::tr("Short");
    }
    return {};
}

// Used in object names, so it stays in English and has no spaces.
QString connectionKey(rf::Connection connection)
{
    switch (connection) {
    case rf::Connection::Series: return "Series";
    case rf::Connection::Shunt: return "Shunt";
    case rf::Connection::ThroughLine: return "Through";
    case rf::Connection::OpenStub: return "Open";
    case rf::Connection::ShortStub: return "Short";
    }
    return {};
}

QIcon symbolIcon(rf::Kind kind, rf::Connection connection, const QPalette& palette, qreal ratio)
{
    auto component = rf::defaultComponent(kind, 1000, 50);
    component.connection = connection;
    QPixmap pixmap((kIconSize * ratio).expandedTo(QSize(1, 1)));
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(kScale, kScale);
    painter.translate(-20, -44);
    painter.setPen(QPen(palette.buttonText().color(), 1.7 / kScale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    paintComponentSymbol(painter, component, false);
    painter.end();
    return QIcon(pixmap);
}
}

ComponentPalette::ComponentPalette(QWidget* parent) : QWidget(parent)
{
    setObjectName("componentPalette");
    const std::vector<Entry> entries{
        {rf::Kind::Capacitor, u'C', {rf::Connection::Series, rf::Connection::Shunt}},
        {rf::Kind::Inductor, u'L', {rf::Connection::Series, rf::Connection::Shunt}},
        {rf::Kind::Resistor, u'R', {rf::Connection::Series, rf::Connection::Shunt}},
        {rf::Kind::Line, u'T',
         {rf::Connection::ThroughLine, rf::Connection::OpenStub, rf::Connection::ShortStub}},
        {rf::Kind::SeriesRlc, u'X', {rf::Connection::Series, rf::Connection::Shunt}},
        {rf::Kind::Transformer, u'I', {rf::Connection::ThroughLine}},
        {rf::Kind::Coax, u'K',
         {rf::Connection::ThroughLine, rf::Connection::OpenStub, rf::Connection::ShortStub}},
        {rf::Kind::Microstrip, u'B',
         {rf::Connection::ThroughLine, rf::Connection::OpenStub, rf::Connection::ShortStub}}};

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);
    for (const auto& entry : entries) {
        const auto name = QString::fromStdString(rf::kindName(entry.kind));
        auto* box = new QGroupBox(name);
        box->setObjectName(QString("palette%1").arg(QString(entry.key)));
        auto* boxLayout = new QHBoxLayout(box);
        boxLayout->setContentsMargins(4, 2, 4, 4);
        boxLayout->setSpacing(2);
        for (std::size_t i = 0; i < entry.connections.size(); ++i) {
            const auto connection = entry.connections[i];
            auto* button = new QToolButton;
            button->setObjectName(QString("palette%1_%2").arg(QString(entry.key), connectionKey(connection)));
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            button->setIconSize(kIconSize);
            button->setText(connectionLabel(connection));
            button->setAutoRaise(true);
            // The first connection of each type is what the keyboard shortcut adds.
            const auto tip = tr("Add %1, %2").arg(name.toLower(), connectionLabel(connection).toLower());
            button->setToolTip(i == 0 ? tr("%1 (%2)").arg(tip, QString(entry.key)) : tip);
            connect(button, &QToolButton::clicked, this,
                    [this, kind = entry.kind, connection] { emit addRequested(kind, connection); });
            boxLayout->addWidget(button);
            buttons_.push_back({button, {entry.kind, connection}});
        }
        row->addWidget(box);
    }
    refreshIcons();
}

void ComponentPalette::refreshIcons()
{
    for (const auto& item : buttons_)
        item.first->setIcon(symbolIcon(item.second.first, item.second.second, palette(), devicePixelRatioF()));
}

void ComponentPalette::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    // Icons are drawn in the current text colour, so redraw them after a theme change.
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        refreshIcons();
}
