// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSEDITORLITEPARAMETERMAPPER_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSEDITORLITEPARAMETERMAPPER_H

#include <string_view>

namespace opendspx {
    struct Model;
}

namespace ImportExportManager::Internal {

    class DsEditorLiteParameterMapper {
    public:
        enum class Direction {
            FromDsEditorLite,
            ToDsEditorLite,
        };

        // Returns the DiffScope document value, or the input for an unknown parameter.
        static int fromDsEditorLite(std::string_view parameterId, int value);
        // Returns the DS Editor Lite document value, or the input for an unknown parameter.
        static int toDsEditorLite(std::string_view parameterId, int value);

        // Maps singing-clip parameters without modifying shared clips or curves in other models.
        static void mapModel(opendspx::Model &model, Direction direction);
    };

}

#endif // DIFFSCOPE_IMPORT_EXPORT_MANAGER_DSEDITORLITEPARAMETERMAPPER_H
