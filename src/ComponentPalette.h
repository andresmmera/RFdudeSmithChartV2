#pragma once

#include "RfModel.h"

#include <QWidget>

#include <utility>
#include <vector>

class QToolButton;

// A row of group boxes, one per component type. Each box holds one icon button per
// connection (series / shunt, or through / open stub / short stub). The icons are drawn
// with the schematic's own symbol code, so they always match what ends up in the schematic.
class ComponentPalette final : public QWidget
{
    Q_OBJECT

public:
    explicit ComponentPalette(QWidget* parent = nullptr);

signals:
    void addRequested(rf::Kind kind, rf::Connection connection);

protected:
    void changeEvent(QEvent* event) override;

private:
    void refreshIcons();
    std::vector<std::pair<QToolButton*, std::pair<rf::Kind, rf::Connection>>> buttons_;
};
