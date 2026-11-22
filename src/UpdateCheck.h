#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rf {

struct ReleaseVersion
{
    int major = 0;
    int minor = 0;
    int patch = 0;
};

std::optional<ReleaseVersion> parseReleaseVersion(std::string_view tag);
bool releaseVersionIsNewer(std::string_view tag, std::string_view currentVersion);

}
