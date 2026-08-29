// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#include "JsonSchemaForm.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSpinBox>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace LibreSVIPFormatConverter::Internal {

    static QJsonValue parseJsonValue(const QString &text, bool *ok) {
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(text.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError) {
            *ok = true;
            return document.isObject() ? QJsonValue(document.object()) : QJsonValue(document.array());
        }
        document = QJsonDocument::fromJson((QStringLiteral("[") + text + QStringLiteral("]")).toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && document.isArray() && document.array().size() == 1) {
            *ok = true;
            return document.array().first();
        }
        *ok = false;
        return {};
    }

    static QString jsonValueText(const QJsonValue &value) {
        if (value.isObject())
            return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Indented));
        if (value.isArray())
            return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Indented));
        QJsonArray wrapper{value};
        QString text = QString::fromUtf8(QJsonDocument(wrapper).toJson(QJsonDocument::Compact));
        return text.mid(1, text.size() - 2);
    }

    struct SchemaField {
        QWidget *widget{};
        bool objectLike{};
        std::function<QJsonValue()> value;
        std::function<void(const QJsonValue &)> setValue;
        std::function<bool(QString *)> validate;
    };

    using SchemaFieldPtr = std::shared_ptr<SchemaField>;

    class DescriptionLabel : public QLabel {
    public:
        explicit DescriptionLabel(const QString &text, QWidget *parent = nullptr) : QLabel(text, parent) {
            setContentsMargins(0, 0, 0, 1);
        }

    protected:
        void paintEvent(QPaintEvent *event) override {
            QLabel::paintEvent(event);

            const QRect textRect = style()->itemTextRect(fontMetrics(), contentsRect(), alignment(), isEnabled(), text());
            if (textRect.isEmpty())
                return;

            QPainter painter(this);
            QPen pen(palette().color(foregroundRole()));
            pen.setStyle(Qt::DotLine);
            pen.setWidth(1);
            pen.setCosmetic(true);
            painter.setPen(pen);
            const int underlineY = qMin(rect().bottom(), textRect.bottom() + 1);
            painter.drawLine(textRect.left(), underlineY, textRect.right(), underlineY);
        }
    };

    class PrimitiveArrayEditor : public QWidget {
        Q_OBJECT
    public:
        using Builder = std::function<SchemaFieldPtr(const QJsonValue &)>;

        PrimitiveArrayEditor(Builder builder, int minimumItems, int maximumItems, bool uniqueItems,
                             const QJsonArray &initial, QWidget *parent = nullptr)
            : QWidget(parent), m_builder(std::move(builder)), m_minimumItems(minimumItems),
              m_maximumItems(maximumItems), m_uniqueItems(uniqueItems) {
            auto layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            m_itemsLayout = new QVBoxLayout;
            layout->addLayout(m_itemsLayout);
            auto buttonRow = new QHBoxLayout;
            m_addButton = new QPushButton(tr("Add"), this);
            buttonRow->addWidget(m_addButton);
            buttonRow->addStretch();
            layout->addLayout(buttonRow);
            connect(m_addButton, &QPushButton::clicked, this, [this] {
                addItem(QJsonValue(QJsonValue::Undefined));
            });
            setValues(initial);
        }

        QJsonArray values() const {
            QJsonArray array;
            for (const auto &row : m_rows)
                array.append(row.field->value());
            return array;
        }

        void setValues(const QJsonArray &array) {
            while (!m_rows.isEmpty())
                removeItem(m_rows.size() - 1);
            for (const auto &value : array)
                addItem(value);
            updateButtons();
        }

        bool validate(QString *errorMessage) const {
            if (m_rows.size() < m_minimumItems) {
                if (errorMessage)
                    *errorMessage = tr("At least %1 item(s) are required.").arg(m_minimumItems);
                return false;
            }
            if (m_maximumItems >= 0 && m_rows.size() > m_maximumItems) {
                if (errorMessage)
                    *errorMessage = tr("No more than %1 item(s) are allowed.").arg(m_maximumItems);
                return false;
            }
            for (const auto &row : m_rows) {
                if (!row.field->validate(errorMessage))
                    return false;
            }
            if (m_uniqueItems) {
                QSet<QByteArray> values;
                for (const auto &value : this->values()) {
                    const auto key = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
                    if (values.contains(key)) {
                        if (errorMessage)
                            *errorMessage = tr("Array items must be unique.");
                        return false;
                    }
                    values.insert(key);
                }
            }
            return true;
        }

    private:
        struct Row {
            QWidget *container{};
            SchemaFieldPtr field;
            QToolButton *up{};
            QToolButton *down{};
            QToolButton *remove{};
        };

        int rowIndex(QWidget *container) const {
            for (int i = 0; i < m_rows.size(); ++i) {
                if (m_rows.at(i).container == container)
                    return i;
            }
            return -1;
        }

        void addItem(const QJsonValue &initial) {
            if (m_maximumItems >= 0 && m_rows.size() >= m_maximumItems)
                return;
            Row row;
            row.container = new QWidget(this);
            auto layout = new QHBoxLayout(row.container);
            layout->setContentsMargins(0, 0, 0, 0);
            row.field = m_builder(initial);
            if (row.field->widget)
                layout->addWidget(row.field->widget, 1);
            else
                layout->addWidget(new QLabel(tr("Fixed value"), row.container), 1);
            row.up = new QToolButton(row.container);
            row.up->setText(QStringLiteral("\u2191"));
            row.up->setToolTip(tr("Move up"));
            row.down = new QToolButton(row.container);
            row.down->setText(QStringLiteral("\u2193"));
            row.down->setToolTip(tr("Move down"));
            row.remove = new QToolButton(row.container);
            row.remove->setText(QStringLiteral("\u00d7"));
            row.remove->setToolTip(tr("Remove"));
            layout->addWidget(row.up);
            layout->addWidget(row.down);
            layout->addWidget(row.remove);
            connect(row.up, &QToolButton::clicked, this, [this, container = row.container] {
                moveItem(rowIndex(container), -1);
            });
            connect(row.down, &QToolButton::clicked, this, [this, container = row.container] {
                moveItem(rowIndex(container), 1);
            });
            connect(row.remove, &QToolButton::clicked, this, [this, container = row.container] {
                removeItem(rowIndex(container));
            });
            m_rows.append(row);
            m_itemsLayout->addWidget(row.container);
            updateButtons();
        }

        void removeItem(int index) {
            if (index < 0 || index >= m_rows.size())
                return;
            auto row = m_rows.takeAt(index);
            m_itemsLayout->removeWidget(row.container);
            delete row.container;
            updateButtons();
        }

        void moveItem(int index, int delta) {
            const int target = index + delta;
            if (index < 0 || target < 0 || target >= m_rows.size())
                return;
            m_rows.move(index, target);
            m_itemsLayout->removeWidget(m_rows.at(target).container);
            m_itemsLayout->insertWidget(target, m_rows.at(target).container);
            updateButtons();
        }

        void updateButtons() {
            for (int i = 0; i < m_rows.size(); ++i) {
                m_rows[i].up->setEnabled(i > 0);
                m_rows[i].down->setEnabled(i + 1 < m_rows.size());
            }
            m_addButton->setEnabled(m_maximumItems < 0 || m_rows.size() < m_maximumItems);
        }

        Builder m_builder;
        int m_minimumItems{};
        int m_maximumItems{-1};
        bool m_uniqueItems{};
        QVBoxLayout *m_itemsLayout{};
        QPushButton *m_addButton{};
        QList<Row> m_rows;
    };

    class JsonSchemaForm::Private {
    public:
        Private(JsonSchemaForm *q, QJsonObject schema) : q(q), rootSchema(std::move(schema)) {
            auto layout = new QVBoxLayout(q);
            layout->setContentsMargins(0, 0, 0, 0);
            rootField = buildField(rootSchema, QJsonValue(QJsonValue::Undefined), QString(), true);
            if (rootField->widget)
                layout->addWidget(rootField->widget);
            layout->addStretch();
        }

        QJsonObject resolveSchema(QJsonObject schema) const {
            QSet<QString> visited;
            while (schema.value(QStringLiteral("$ref")).isString()) {
                const QString reference = schema.value(QStringLiteral("$ref")).toString();
                if (!reference.startsWith(QStringLiteral("#/")) || visited.contains(reference))
                    break;
                visited.insert(reference);
                QJsonValue resolved(rootSchema);
                const auto parts = reference.mid(2).split(QLatin1Char('/'));
                for (QString part : parts) {
                    part.replace(QStringLiteral("~1"), QStringLiteral("/"));
                    part.replace(QStringLiteral("~0"), QStringLiteral("~"));
                    resolved = resolved.toObject().value(part);
                }
                if (!resolved.isObject())
                    break;
                QJsonObject merged = resolved.toObject();
                for (auto it = schema.begin(); it != schema.end(); ++it) {
                    if (it.key() != QStringLiteral("$ref"))
                        merged.insert(it.key(), it.value());
                }
                schema = merged;
            }
            return schema;
        }

        QWidget *makeLabel(const QString &text, const QString &description, QWidget *parent) const {
            auto container = new QWidget(parent);
            auto layout = new QHBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            QLabel *label;
            if (!description.isEmpty()) {
                label = new DescriptionLabel(text, container);
                label->setCursor(Qt::WhatsThisCursor);
                label->setToolTip(description);
                label->setAccessibleDescription(description);
            } else {
                label = new QLabel(text, container);
            }
            layout->addWidget(label);
            layout->addStretch();
            return container;
        }

        bool enumOptionTexts(const QJsonObject &schema, QStringList *optionTexts) const {
            if (!schema.value(QStringLiteral("enum")).isArray())
                return false;

            const QJsonArray values = schema.value(QStringLiteral("enum")).toArray();
            optionTexts->clear();
            if (!schema.contains(QStringLiteral("oneOf"))) {
                for (const auto &value : values)
                    optionTexts->append(value.isString() ? value.toString() : jsonValueText(value));
                return true;
            }

            if (!schema.value(QStringLiteral("oneOf")).isArray())
                return false;
            const QJsonArray alternatives = schema.value(QStringLiteral("oneOf")).toArray();
            if (alternatives.size() != values.size())
                return false;

            QList<bool> usedAlternatives(alternatives.size(), false);
            for (const auto &value : values) {
                int matchingIndex = -1;
                QJsonObject matchingAlternative;
                for (int i = 0; i < alternatives.size(); ++i) {
                    if (usedAlternatives.at(i) || !alternatives.at(i).isObject())
                        continue;
                    const QJsonObject alternative = resolveSchema(alternatives.at(i).toObject());
                    if (alternative.contains(QStringLiteral("const")) &&
                        alternative.value(QStringLiteral("const")) == value) {
                        matchingIndex = i;
                        matchingAlternative = alternative;
                        break;
                    }
                }
                if (matchingIndex < 0) {
                    optionTexts->clear();
                    return false;
                }
                usedAlternatives[matchingIndex] = true;
                const QString alternativeTitle = matchingAlternative.value(QStringLiteral("title")).toString();
                optionTexts->append(alternativeTitle.isEmpty()
                                        ? (value.isString() ? value.toString() : jsonValueText(value))
                                        : alternativeTitle);
            }
            return true;
        }

        SchemaFieldPtr buildJsonEditor(const QJsonValue &initial, const QString &type,
                                       const QString &title) const {
            auto editor = new QPlainTextEdit(q);
            editor->setMinimumHeight(120);
            QJsonValue fallbackInitial = initial;
            if (fallbackInitial.isUndefined())
                fallbackInitial = type == QStringLiteral("array") ? QJsonValue(QJsonArray{}) : QJsonValue(QJsonObject{});
            editor->setPlainText(jsonValueText(fallbackInitial));
            return std::make_shared<SchemaField>(SchemaField{
                editor, false,
                [editor] {
                    bool ok = false;
                    const auto value = parseJsonValue(editor->toPlainText(), &ok);
                    return ok ? value : QJsonValue();
                },
                [editor](const QJsonValue &value) { editor->setPlainText(jsonValueText(value)); },
                [editor, title](QString *error) {
                    bool ok = false;
                    parseJsonValue(editor->toPlainText(), &ok);
                    if (!ok && error)
                        *error = tr("%1 must contain valid JSON.").arg(title);
                    return ok;
                }
            });
        }

        SchemaFieldPtr buildField(const QJsonObject &rawSchema, const QJsonValue &providedInitial,
                                  const QString &propertyName, bool root = false) {
            const QJsonObject schema = resolveSchema(rawSchema);
            QJsonValue initial = providedInitial;
            if (initial.isUndefined() && schema.contains(QStringLiteral("default")))
                initial = schema.value(QStringLiteral("default"));
            const QString title = schema.value(QStringLiteral("title")).toString(propertyName);

            if (schema.contains(QStringLiteral("const"))) {
                const QJsonValue fixed = schema.value(QStringLiteral("const"));
                return std::make_shared<SchemaField>(SchemaField{
                    nullptr, false, [fixed] { return fixed; }, [](const QJsonValue &) {}, [](QString *) { return true; }
                });
            }

            const QString type = schema.value(QStringLiteral("type")).toString();
            if (type == QStringLiteral("null")) {
                return std::make_shared<SchemaField>(SchemaField{
                    nullptr, false, [] { return QJsonValue(QJsonValue::Null); }, [](const QJsonValue &) {}, [](QString *) { return true; }
                });
            }

            QStringList enumTexts;
            if (enumOptionTexts(schema, &enumTexts)) {
                auto combo = new QComboBox(q);
                const auto values = schema.value(QStringLiteral("enum")).toArray();
                for (int i = 0; i < values.size(); ++i)
                    combo->addItem(enumTexts.at(i), values.at(i).toVariant());
                const QJsonValue defaultValue = initial.isUndefined() && !values.isEmpty() ? values.first() : initial;
                for (int i = 0; i < combo->count(); ++i) {
                    if (QJsonValue::fromVariant(combo->itemData(i)) == defaultValue) {
                        combo->setCurrentIndex(i);
                        break;
                    }
                }
                return std::make_shared<SchemaField>(SchemaField{
                    combo, false,
                    [combo] { return QJsonValue::fromVariant(combo->currentData()); },
                    [combo](const QJsonValue &value) {
                        for (int i = 0; i < combo->count(); ++i) {
                            if (QJsonValue::fromVariant(combo->itemData(i)) == value) {
                                combo->setCurrentIndex(i);
                                return;
                            }
                        }
                    },
                    [combo, title](QString *error) {
                        if (combo->currentIndex() >= 0)
                            return true;
                        if (error) *error = tr("%1 requires a value.").arg(title);
                        return false;
                    }
                });
            }

            if (schema.contains(QStringLiteral("enum")) || schema.contains(QStringLiteral("oneOf")))
                return buildJsonEditor(initial, type, title);

            if (type == QStringLiteral("string")) {
                auto editor = new QLineEdit(q);
                if (initial.isString())
                    editor->setText(initial.toString());
                const int maxLength = schema.value(QStringLiteral("maxLength")).toInt(-1);
                if (maxLength >= 0)
                    editor->setMaxLength(maxLength);
                const QString pattern = schema.value(QStringLiteral("pattern")).toString();
                const QRegularExpression expression(pattern);
                const int minLength = schema.value(QStringLiteral("minLength")).toInt(0);
                return std::make_shared<SchemaField>(SchemaField{
                    editor, false, [editor] { return QJsonValue(editor->text()); },
                    [editor](const QJsonValue &value) { editor->setText(value.toString()); },
                    [editor, title, minLength, maxLength, pattern, expression](QString *error) {
                        if (editor->text().size() < minLength) {
                            if (error) *error = tr("%1 must contain at least %2 character(s).").arg(title).arg(minLength);
                            return false;
                        }
                        if (maxLength >= 0 && editor->text().size() > maxLength) {
                            if (error) *error = tr("%1 must contain no more than %2 character(s).").arg(title).arg(maxLength);
                            return false;
                        }
                        if (!pattern.isEmpty() && (!expression.isValid() || !expression.match(editor->text()).hasMatch())) {
                            if (error) *error = tr("%1 does not match the required pattern.").arg(title);
                            return false;
                        }
                        return true;
                    }
                });
            }

            if (type == QStringLiteral("boolean")) {
                auto editor = new QCheckBox(q);
                editor->setChecked(initial.isBool() ? initial.toBool() : false);
                return std::make_shared<SchemaField>(SchemaField{
                    editor, false, [editor] { return QJsonValue(editor->isChecked()); },
                    [editor](const QJsonValue &value) { editor->setChecked(value.toBool()); }, [](QString *) { return true; }
                });
            }

            if (type == QStringLiteral("integer")) {
                auto editor = new QSpinBox(q);
                const bool hasMinimum = schema.contains(QStringLiteral("minimum"));
                const bool hasMaximum = schema.contains(QStringLiteral("maximum"));
                const bool hasExclusiveMinimum = schema.contains(QStringLiteral("exclusiveMinimum"));
                const bool hasExclusiveMaximum = schema.contains(QStringLiteral("exclusiveMaximum"));
                int minimum = std::numeric_limits<int>::min();
                int maximum = std::numeric_limits<int>::max();
                if (hasMinimum) {
                    minimum = std::max(minimum, static_cast<int>(std::ceil(schema.value(QStringLiteral("minimum")).toDouble())));
                }
                if (hasExclusiveMinimum) {
                    const double limit = schema.value(QStringLiteral("exclusiveMinimum")).toDouble();
                    minimum = std::max(minimum, static_cast<int>(std::floor(limit)) + 1);
                }
                if (hasMaximum) {
                    maximum = std::min(maximum, static_cast<int>(std::floor(schema.value(QStringLiteral("maximum")).toDouble())));
                }
                if (hasExclusiveMaximum) {
                    const double limit = schema.value(QStringLiteral("exclusiveMaximum")).toDouble();
                    maximum = std::min(maximum, static_cast<int>(std::ceil(limit)) - 1);
                }
                editor->setRange(minimum, maximum);
                editor->setSingleStep(std::max(1, schema.value(QStringLiteral("multipleOf")).toInt(1)));
                editor->setValue(initial.isDouble() ? initial.toInt() : std::clamp(0, minimum, maximum));
                const int multipleOf = schema.value(QStringLiteral("multipleOf")).toInt(0);
                return std::make_shared<SchemaField>(SchemaField{
                    editor, false, [editor] { return QJsonValue(editor->value()); },
                    [editor](const QJsonValue &value) { editor->setValue(value.toInt()); },
                    [editor, title, multipleOf](QString *error) {
                        if (multipleOf > 0 && editor->value() % multipleOf != 0) {
                            if (error) *error = tr("%1 must be a multiple of %2.").arg(title).arg(multipleOf);
                            return false;
                        }
                        return true;
                    }
                });
            }

            if (type == QStringLiteral("number")) {
                auto editor = new QDoubleSpinBox(q);
                editor->setDecimals(8);
                const bool hasMinimum = schema.contains(QStringLiteral("minimum"));
                const bool hasMaximum = schema.contains(QStringLiteral("maximum"));
                const bool hasExclusiveMinimum = schema.contains(QStringLiteral("exclusiveMinimum"));
                const bool hasExclusiveMaximum = schema.contains(QStringLiteral("exclusiveMaximum"));
                const double minimumLimit = schema.value(QStringLiteral("minimum")).toDouble();
                const double maximumLimit = schema.value(QStringLiteral("maximum")).toDouble();
                const double exclusiveMinimumLimit = schema.value(QStringLiteral("exclusiveMinimum")).toDouble();
                const double exclusiveMaximumLimit = schema.value(QStringLiteral("exclusiveMaximum")).toDouble();
                double minimum = schema.value(QStringLiteral("minimum")).toDouble(-1e100);
                double maximum = schema.value(QStringLiteral("maximum")).toDouble(1e100);
                if (hasExclusiveMinimum)
                    minimum = std::max(minimum, std::nextafter(exclusiveMinimumLimit, std::numeric_limits<double>::infinity()));
                if (hasExclusiveMaximum)
                    maximum = std::min(maximum, std::nextafter(exclusiveMaximumLimit, -std::numeric_limits<double>::infinity()));
                editor->setRange(minimum, maximum);
                const double multipleOf = schema.value(QStringLiteral("multipleOf")).toDouble(0);
                editor->setSingleStep(multipleOf > 0 ? multipleOf : 1.0);
                editor->setValue(initial.isDouble() ? initial.toDouble() : std::clamp(0.0, minimum, maximum));
                return std::make_shared<SchemaField>(SchemaField{
                    editor, false, [editor] { return QJsonValue(editor->value()); },
                    [editor](const QJsonValue &value) { editor->setValue(value.toDouble()); },
                    [editor, title, multipleOf, hasMinimum, hasMaximum, hasExclusiveMinimum, hasExclusiveMaximum,
                     minimumLimit, maximumLimit, exclusiveMinimumLimit, exclusiveMaximumLimit](QString *error) {
                        const double value = editor->value();
                        if ((hasMinimum && value < minimumLimit) ||
                            (hasExclusiveMinimum && value <= exclusiveMinimumLimit)) {
                            if (error) *error = tr("%1 is below the allowed minimum.").arg(title);
                            return false;
                        }
                        if ((hasMaximum && value > maximumLimit) ||
                            (hasExclusiveMaximum && value >= exclusiveMaximumLimit)) {
                            if (error) *error = tr("%1 is above the allowed maximum.").arg(title);
                            return false;
                        }
                        if (multipleOf > 0) {
                            const double quotient = value / multipleOf;
                            if (std::abs(quotient - std::round(quotient)) > 1e-8) {
                                if (error) *error = tr("%1 must be a multiple of %2.").arg(title).arg(multipleOf);
                                return false;
                            }
                        }
                        return true;
                    }
                });
            }

            if (type == QStringLiteral("object")) {
                QWidget *container{};
                QFormLayout *layout{};
                if (root) {
                    container = new QWidget(q);
                    layout = new QFormLayout(container);
                    layout->setContentsMargins(0, 0, 0, 0);
                } else {
                    auto group = new QGroupBox(q);
                    auto groupLayout = new QVBoxLayout(group);
                    groupLayout->addWidget(makeLabel(title, schema.value(QStringLiteral("description")).toString(), group));
                    auto content = new QWidget(group);
                    layout = new QFormLayout(content);
                    groupLayout->addWidget(content);
                    container = group;
                }
                QList<QPair<QString, SchemaFieldPtr>> children;
                const auto properties = schema.value(QStringLiteral("properties")).toObject();
                const QJsonObject initialObject = initial.toObject();
                for (auto it = properties.begin(); it != properties.end(); ++it) {
                    if (!it.value().isObject())
                        continue;
                    const auto childSchema = resolveSchema(it.value().toObject());
                    const QString childTitle = childSchema.value(QStringLiteral("title")).toString(it.key());
                    auto child = buildField(
                        it.value().toObject(),
                        initialObject.contains(it.key())
                            ? initialObject.value(it.key())
                            : QJsonValue(QJsonValue::Undefined),
                        it.key());
                    children.append({it.key(), child});
                    if (!child->widget)
                        continue;
                    if (child->objectLike) {
                        layout->addRow(child->widget);
                    } else {
                        layout->addRow(makeLabel(childTitle, childSchema.value(QStringLiteral("description")).toString(), container), child->widget);
                    }
                }
                auto field = std::make_shared<SchemaField>();
                field->widget = container;
                field->objectLike = !root;
                field->value = [children] {
                    QJsonObject object;
                    for (const auto &child : children)
                        object.insert(child.first, child.second->value());
                    return QJsonValue(object);
                };
                field->setValue = [children](const QJsonValue &value) {
                    const auto object = value.toObject();
                    for (const auto &child : children) {
                        if (object.contains(child.first))
                            child.second->setValue(object.value(child.first));
                    }
                };
                field->validate = [children](QString *error) {
                    for (const auto &child : children) {
                        QString childError;
                        if (!child.second->validate(&childError)) {
                            if (error) *error = childError;
                            return false;
                        }
                    }
                    return true;
                };
                return field;
            }

            if (type == QStringLiteral("array")) {
                const auto itemSchema = resolveSchema(schema.value(QStringLiteral("items")).toObject());
                const QString itemType = itemSchema.value(QStringLiteral("type")).toString();
                QStringList itemEnumTexts;
                const bool simpleEnum = enumOptionTexts(itemSchema, &itemEnumTexts);
                const bool hasAlternatives = itemSchema.contains(QStringLiteral("enum")) ||
                                             itemSchema.contains(QStringLiteral("oneOf"));
                const bool simple = itemSchema.contains(QStringLiteral("const")) || simpleEnum ||
                                    (!hasAlternatives &&
                                     (itemType == QStringLiteral("string") || itemType == QStringLiteral("boolean") ||
                                      itemType == QStringLiteral("integer") || itemType == QStringLiteral("number") ||
                                      itemType == QStringLiteral("null")));
                if (simple) {
                    auto editor = new PrimitiveArrayEditor(
                        [this, itemSchema](const QJsonValue &value) { return buildField(itemSchema, value, QString()); },
                        schema.value(QStringLiteral("minItems")).toInt(0),
                        schema.contains(QStringLiteral("maxItems")) ? schema.value(QStringLiteral("maxItems")).toInt() : -1,
                        schema.value(QStringLiteral("uniqueItems")).toBool(false),
                        initial.toArray(), q);
                    return std::make_shared<SchemaField>(SchemaField{
                        editor, false, [editor] { return QJsonValue(editor->values()); },
                        [editor](const QJsonValue &value) { editor->setValues(value.toArray()); },
                        [editor, title](QString *error) {
                            QString detail;
                            if (editor->validate(&detail))
                                return true;
                            if (error) *error = title.isEmpty() ? detail : tr("%1: %2").arg(title, detail);
                            return false;
                        }
                    });
                }
            }

            return buildJsonEditor(initial, type, title);
        }

        JsonSchemaForm *q;
        QJsonObject rootSchema;
        SchemaFieldPtr rootField;
    };

    JsonSchemaForm::JsonSchemaForm(const QJsonObject &schema, QWidget *parent)
        : QWidget(parent), d(std::make_unique<Private>(this, schema)) {
    }

    JsonSchemaForm::~JsonSchemaForm() = default;

    QJsonObject JsonSchemaForm::value() const {
        return d->rootField->value().toObject();
    }

    void JsonSchemaForm::setValue(const QJsonObject &value) {
        d->rootField->setValue(value);
    }

    bool JsonSchemaForm::validate(QString *errorMessage) const {
        return d->rootField->validate(errorMessage);
    }

}

#include "JsonSchemaForm.moc"
