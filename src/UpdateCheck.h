#pragma once

#include <cstdint>
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

struct BuildTag
{
    std::uint64_t run = 0;
    unsigned attempt = 0;
};

std::optional<ReleaseVersion> parseReleaseVersion(std::string_view tag);
std::optional<ReleaseVersion> parseReleaseVersionInText(std::string_view text);
std::optional<BuildTag> parseBuildTag(std::string_view tag);
bool releaseVersionIsNewer(std::string_view tag, std::string_view currentVersion);
bool buildTagIsNewer(std::string_view candidateTag, std::string_view currentTag);

}
