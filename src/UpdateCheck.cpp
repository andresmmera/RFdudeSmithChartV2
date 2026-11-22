#include "UpdateCheck.h"

#include <charconv>
#include <system_error>

namespace rf {

std::optional<ReleaseVersion> parseReleaseVersion(std::string_view tag)
{
    if (!tag.empty() && tag.front() == 'v') tag.remove_prefix(1);
    const auto parsePart = [](std::string_view text, int& value) {
        if (text.empty() || text.front() < '0' || text.front() > '9') return false;
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

std::optional<ReleaseVersion> parseReleaseVersionInText(std::string_view text)
{
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') continue;
        if (i > 0) {
            const char previous = text[i - 1];
            if (previous == '.' || (previous >= '0' && previous <= '9')) continue;
        }
        const auto firstDot = text.find('.', i);
        if (firstDot == std::string_view::npos) continue;
        const auto secondDot = text.find('.', firstDot + 1);
        if (secondDot == std::string_view::npos) continue;
        std::size_t patchEnd = secondDot + 1;
        while (patchEnd < text.size()
               && text[patchEnd] >= '0' && text[patchEnd] <= '9')
            ++patchEnd;
        if (patchEnd == secondDot + 1) continue;
        if (patchEnd < text.size()) {
            const char next = text[patchEnd];
            if ((next >= '0' && next <= '9') || next == '.'
                || (next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z'))
                continue;
        }
        if (const auto version = parseReleaseVersion(text.substr(i, patchEnd - i)))
            return version;
    }
    return std::nullopt;
}

std::optional<BuildTag> parseBuildTag(std::string_view tag)
{
    constexpr std::string_view prefix = "build-";
    if (!tag.starts_with(prefix)) return std::nullopt;
    tag.remove_prefix(prefix.size());
    const auto separator = tag.find('-');
    if (separator == std::string_view::npos) return std::nullopt;
    const auto runText = tag.substr(0, separator);
    const auto attemptText = tag.substr(separator + 1);
    if (runText.empty() || attemptText.empty()) return std::nullopt;

    std::uint64_t run = 0;
    const char* runFirst = runText.data();
    const char* runLast = runFirst + runText.size();
    const auto runResult = std::from_chars(runFirst, runLast, run);
    if (runResult.ec != std::errc{} || runResult.ptr != runLast || run == 0)
        return std::nullopt;

    unsigned attempt = 0;
    const char* attemptFirst = attemptText.data();
    const char* attemptLast = attemptFirst + attemptText.size();
    const auto attemptResult = std::from_chars(attemptFirst, attemptLast, attempt);
    if (attemptResult.ec != std::errc{} || attemptResult.ptr != attemptLast || attempt == 0)
        return std::nullopt;
    return BuildTag{run, attempt};
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

bool buildTagIsNewer(std::string_view candidateTag, std::string_view currentTag)
{
    const auto candidate = parseBuildTag(candidateTag);
    const auto current = parseBuildTag(currentTag);
    if (!candidate || !current) return false;
    if (candidate->run != current->run) return candidate->run > current->run;
    return candidate->attempt > current->attempt;
}

}
