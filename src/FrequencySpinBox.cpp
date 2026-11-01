#include "FrequencySpinBox.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLocale>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QWidget>

#include <algorithm>

FrequencySpinBox::FrequencySpinBox(QWidget* parent) : QDoubleSpinBox(parent)
{
    // Internal resolution is 1e-9 MHz (1 mHz); the displayed text is trimmed, not fixed-width.
    setDecimals(9);
    setRange(1e-9, 1e12);
    setKeyboardTracking(false);
    setSingleStep(1.0);
}

double FrequencySpinBox::toDisplay(double megahertz) const
{
    switch (unit_) {
    case Unit::kHz: return megahertz * 1000.0;
    case Unit::GHz: return megahertz / 1000.0;
    case Unit::MHz: break;
    }
    return megahertz;
}

double FrequencySpinBox::fromDisplay(double shown) const
{
    switch (unit_) {
    case Unit::kHz: return shown / 1000.0;
    case Unit::GHz: return shown * 1000.0;
    case Unit::MHz: break;
    }
    return shown;
}

FrequencySpinBox::Unit FrequencySpinBox::preferredUnit(double megahertz)
{
    if (!(megahertz > 0)) return Unit::MHz;
    if (megahertz < 1.0) return Unit::kHz;
    if (megahertz >= 1000.0) return Unit::GHz;
    return Unit::MHz;
}

void FrequencySpinBox::setUnit(Unit unit)
{
    unit_ = unit;
    // The arrows / wheel step by one displayed unit (1 kHz, 1 MHz or 1 GHz).
    setSingleStep(fromDisplay(1.0));
    // Re-render the text in the new unit; the value itself (in MHz) does not change.
    const QSignalBlocker blocker(this);
    setValue(value());
}

QString FrequencySpinBox::textFromValue(double megahertz) const
{
    QLocale plain = locale();
    plain.setNumberOptions(plain.numberOptions() | QLocale::OmitGroupSeparator);
    QString text = plain.toString(toDisplay(megahertz), 'f', 12);
    const QString point = plain.decimalPoint();
    if (text.contains(point)) {
        while (text.endsWith(QLatin1Char('0'))) text.chop(1);
        if (text.endsWith(point)) text.chop(point.size());
    }
    return text;
}

double FrequencySpinBox::valueFromText(const QString& text) const
{
    bool ok = false;
    const double shown = locale().toDouble(text.trimmed(), &ok);
    if (!ok) return value();
    return std::clamp(fromDisplay(shown), minimum(), maximum());
}

QValidator::State FrequencySpinBox::validate(QString& text, int&) const
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return QValidator::Intermediate;
    bool ok = false;
    locale().toDouble(trimmed, &ok);
    if (ok) return QValidator::Acceptable;
    static const QRegularExpression partial(QStringLiteral("^[0-9.,eE+-]*$"));
    return partial.match(trimmed).hasMatch() ? QValidator::Intermediate : QValidator::Invalid;
}

QWidget* frequencyEditor(FrequencySpinBox* spin)
{
    spin->setUnit(FrequencySpinBox::preferredUnit(spin->value()));
    auto* holder = new QWidget;
    auto* row = new QHBoxLayout(holder);
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(spin, 1);
    auto* units = new QComboBox;
    units->setObjectName(spin->objectName() + QStringLiteral("Unit"));
    units->addItems({QStringLiteral("kHz"), QStringLiteral("MHz"), QStringLiteral("GHz")});
    units->setCurrentIndex(static_cast<int>(spin->unit()));
    row->addWidget(units);
    QObject::connect(units, &QComboBox::currentIndexChanged, spin, [spin](int index) {
        spin->setUnit(static_cast<FrequencySpinBox::Unit>(index));
    });
    return holder;
}

FrequencySpinBox* addFrequencyRow(QFormLayout* form, const QString& label,
                                  const QString& name, double valueMHz)
{
    auto* spin = new FrequencySpinBox;
    spin->setObjectName(name);
    spin->setValue(valueMHz);
    form->addRow(label, frequencyEditor(spin));
    return spin;
}