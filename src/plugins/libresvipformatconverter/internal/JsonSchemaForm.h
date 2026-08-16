// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_JSONSCHEMAFORM_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_JSONSCHEMAFORM_H

#include <QJsonObject>
#include <QWidget>

#include <memory>

namespace LibreSVIPFormatConverter::Internal {

    class JsonSchemaForm : public QWidget {
        Q_OBJECT
    public:
        explicit JsonSchemaForm(const QJsonObject &schema, QWidget *parent = nullptr);
        ~JsonSchemaForm() override;

        QJsonObject value() const;
        void setValue(const QJsonObject &value);
        bool validate(QString *errorMessage = nullptr) const;

    private:
        class Private;
        std::unique_ptr<Private> d;
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_JSONSCHEMAFORM_H
