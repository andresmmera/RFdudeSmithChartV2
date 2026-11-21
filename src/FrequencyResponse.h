#pragma once

#include "RfModel.h"

#include <limits>

namespace rf {

struct Sweep
{
    double startMHz = 100;
    double stopMHz = 2000;
    int points = 401;
    bool logarithmic = false;
    double portOhms = 50;
};

struct StabilityMetrics
{
    bool kDefined = false;
    bool gmaxDefined = false;
    bool msgDefined = false;
    double k = 0;
    double deltaMagnitude = 0;
    double b1 = 0;
    double gmaxDb = -std::numeric_limits<double>::infinity();
    double msgDb = -std::numeric_limits<double>::infinity();
};

struct StabilityCircle
{
    Complex center{0, 0};
    double radius = 0;
    bool valid = false;
    bool originStable = false;
    bool originInside = false;
    bool originOutside = false;
};

struct StabilityCircles
{
    StabilityCircle source;
    StabilityCircle load;
};

// Equal, real power-wave references at both ports. Port 1 is the source/input.
// The starting and goal impedances are external terminations, not part of this two-port.
SParameters sParameters(const std::vector<Component>& components, double frequencyMHz,
                        double portOhms);
Complex inputReflection(const Document& document, double frequencyMHz, double portOhms);
StabilityMetrics stabilityMetrics(const SParameters& s);
StabilityCircles stabilityCircles(const SParameters& s);
void validate(const Sweep& sweep);
std::vector<ResponseSample> frequencyResponse(const std::vector<Component>& components,
                                              const Sweep& sweep);

}
