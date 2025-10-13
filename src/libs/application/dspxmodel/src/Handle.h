#ifndef DIFFSCOPE_DSPX_MODEL_HANDLE_H
#define DIFFSCOPE_DSPX_MODEL_HANDLE_H

#include <dspxmodel/DspxModelGlobal.h>


namespace dspx {

    struct Handle {
        quintptr d;

        auto operator<=>(const Handle &) const = default;
        bool operator==(const Handle &) const = default;
        bool operator!=(const Handle &) const = default;

        friend size_t qHash(const Handle &handle, size_t seed = 0) {
            return qHash(handle.d, seed);
        }

        friend QDebug operator<<(QDebug debug, const Handle &handle);

        constexpr bool isNull() const {
            return !d;
        }
        constexpr operator bool() const {
            return d;
        }

    };

}

namespace std {
    template<>
    struct hash<dspx::Handle> {
        size_t operator()(const dspx::Handle &handle) const noexcept {
            return std::hash<quintptr>{}(handle.d);
        }
    };
}

#endif //DIFFSCOPE_DSPX_MODEL_HANDLE_H
