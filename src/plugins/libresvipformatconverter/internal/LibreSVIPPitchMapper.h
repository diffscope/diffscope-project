// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPPITCHMAPPER_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPPITCHMAPPER_H

namespace opendspx {
    struct Model;
}

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPPitchMapper {
    public:
        enum class Direction {
            FromLibreSVIP,
            ToLibreSVIP,
        };

        // Maps pitch curves without modifying shared clips or curves in other models.
        // Applies to all LibreSVIP versions until a version with normalized DSPX pitch values is identified.
        static void mapModel(opendspx::Model &model, Direction direction);
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPPITCHMAPPER_H
