#ifndef DIFFSCOPE_SYNTH_INTERNAL_APIVERSION_H
#define DIFFSCOPE_SYNTH_INTERNAL_APIVERSION_H

#include <optional>

#include <QString>

namespace Synth::Internal::Api {

    enum class ApiVersion : int {
        V1 = 1,
    };

    inline std::optional<ApiVersion> negotiateApiVersion(int serviceMaximumVersion) {
        // Add newer versions from highest to lowest as api/vN implementations become available.
        if (serviceMaximumVersion >= static_cast<int>(ApiVersion::V1))
            return ApiVersion::V1;
        return std::nullopt;
    }

    inline QString apiVersionPrefix(ApiVersion version) {
        return QStringLiteral("/v%1").arg(static_cast<int>(version));
    }

} // namespace Synth::Internal::Api

#endif // DIFFSCOPE_SYNTH_INTERNAL_APIVERSION_H
