#pragma once

#include "RfModel.h"

namespace rf {

/// @brief Topology of a MatchMe network.
enum class MatchType {
    LowPass,      ///< Lumped low-pass L sections.
    HighPass,     ///< Lumped high-pass L sections.
    QuarterWave,  ///< Quarter-wave transformer sections.
    SingleStub,   ///< Series line followed by a shunt stub.
    DoubleStub,   ///< Shunt stub, lambda/8 line, second shunt stub.
    Pi,           ///< Shunt, series, shunt network through a virtual resistance.
    Tee,          ///< Series, shunt, series network through a virtual resistance.
    Tapped        ///< Tapped-C or tapped-L shunt, series, shunt network.
};

/// @brief Termination of the stubs of a stub match.
enum class StubTermination {
    Open,    ///< Open-circuited stub.
    Shorted  ///< Short-circuited stub.
};

/// @brief Element that matches the start reactance in the quarter-wave topology.
enum class ReactanceMatch {
    Lumped,       ///< Lumped series inductor or capacitor.
    LambdaEighth  ///< Lambda/8 line with Z = |Zstart|, then quarter-wave sections.
};

/// @brief Band of each half of a Pi or Tee network, goal (source) side first.
enum class HalfBands {
    LpLp,  ///< Low-pass goal half, low-pass start half.
    LpHp,  ///< Low-pass goal half, high-pass start half.
    HpLp,  ///< High-pass goal half, low-pass start half.
    HpHp   ///< High-pass goal half, high-pass start half.
};

/// @brief Element that is tapped in the tapped-resonator network.
enum class TapElement {
    Capacitor,  ///< Tapped-C: shunt inductor on the higher-resistance side.
    Inductor    ///< Tapped-L: shunt capacitor on the higher-resistance side.
};

struct MatchSpecification
{
    MatchType type = MatchType::LowPass;
    int sections = 1;  ///< Number of sections; ignored by the stub, Pi, Tee and tapped topologies.
    StubTermination stub = StubTermination::Open;  ///< Stub termination; used only by the stub topologies.
    ReactanceMatch startReactance = ReactanceMatch::Lumped;  ///< Start reactance match; used only by QuarterWave.
    double q = 5;  ///< Loaded Q; used by Pi, Tee and Tapped.
    HalfBands halves = HalfBands::LpLp;  ///< Band of each half; used only by Pi and Tee.
    TapElement tap = TapElement::Capacitor;  ///< Tapped element; used only by Tapped.
    Complex startOhms = {5, 0};
    Complex goalOhms = {50, 0};
    double frequencyMHz = 1000;
    double referenceOhms = 50;
};

Document synthesizeMatch(const MatchSpecification& specification);

enum class FilterType { LowPass, HighPass };

struct FilterSpecification
{
    FilterType type = FilterType::LowPass;
    int order = 3;
    double cutoffMHz = 1000;
    double impedanceOhms = 50;
    double rippleDb = 0.1;
    bool shuntFirst = true;
};

Document synthesizeFilter(const FilterSpecification& specification);

}