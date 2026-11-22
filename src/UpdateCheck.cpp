#include "UpdateCheck.h"

#include <charconv>
#include <system_error>

namespace rf {

std::optional<ReleaseVersion> parseReleaseVersion(std::string_view tag)
{
    if (!tag.empty() && tag.front() == 'v') tag.remove_prefix(1);
    const auto parsePart = [](std::string_view text, int& value) {
        if (text.empty()) return false;
        const char* first = text.data();
        const char* last = first + text.size();
        value = 0;
        const auto result = std::from_chars(first, last, value);
        return result.ec == std::errc{} && result.ptr == last;
    };
    const auto firstDot = tag.find('.');
    if (firstDot == std::string_view::npos) return std::nullopt;
    const auto secondDot = tag.find('.', firstDot + 1);
    if (secondDot == std::string_view::npos) return std::nullopt;
    if (tag.find('.', secondDot + 1) != std::string_view::npos) return std::nullopt;
    ReleaseVersion version;
    if (!parsePart(tag.substr(0, firstDot), version.major)
        || !parsePart(tag.substr(firstDot + 1, secondDot - firstDot - 1), version.minor)
        || !parsePart(tag.substr(secondDot + 1), version.patch))
        return std::nullopt;
    return version;
}

bool releaseVersionIsNewer(std::string_view tag, std::string_view currentVersion)
{
    const auto remote = parseReleaseVersion(tag);
    const auto local = parseReleaseVersion(currentVersion);
    if (!remote || !local) return false;
    if (remote->major != local->major) return remote->major > local->major;
    if (remote->minor != local->minor) return remote->minor > local->minor;
    return remote->patch > local->patch;
}

}
