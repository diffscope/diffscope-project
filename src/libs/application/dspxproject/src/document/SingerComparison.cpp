// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SingerComparison.h"

#include <opendspx/mixedsinger.h>
#include <opendspx/singlesinger.h>

namespace DspxProject {

    bool singerDefinitionEquals(const opendspx::SingerRef &left, const opendspx::SingerRef &right) {
        if (!left || !right)
            return left == right;
        if (left->type != right->type || left->extra != right->extra || left->workspace != right->workspace)
            return false;
        if (left->type == opendspx::Singer::Type::Single) {
            return static_cast<const opendspx::SingleSinger &>(*left).id
                   == static_cast<const opendspx::SingleSinger &>(*right).id;
        }
        const auto &leftMixed = static_cast<const opendspx::MixedSinger &>(*left);
        const auto &rightMixed = static_cast<const opendspx::MixedSinger &>(*right);
        if (leftMixed.ratio != rightMixed.ratio || leftMixed.singers.size() != rightMixed.singers.size())
            return false;
        for (std::size_t i = 0; i < leftMixed.singers.size(); ++i) {
            if (!singerDefinitionEquals(leftMixed.singers[i], rightMixed.singers[i]))
                return false;
        }
        return true;
    }

}
