// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JavaScriptDebugConsoleDialog.h"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QSizeF>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTextDocument>
#include <QTextOption>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>
#include <QtMath>

#include <SVSCraftFluentSystemIcons/FluentSystemIcons.h>
#include <SVSCraftGui/DesktopServices.h>
#include <SVSCraftQuick/Theme.h>

#include <batchprocess/JavaScriptConsoleInterface.h>

namespace BatchProcess::Internal {

    namespace {

        constexpr int HorizontalMargin = 8;
        constexpr int VerticalMargin = 5;
        constexpr int IconExtent = 16;
        constexpr int IconColumnWidth = 20;
        constexpr int ContentSpacing = 8;

        QString fileInformation(const QModelIndex &index) {
            const auto fileUrl = index.data(JavaScriptConsoleInterface::FileUrlRole).toUrl();
            if (fileUrl.isEmpty()) {
                return {};
            }

            auto fileName = fileUrl.isLocalFile() ? QFileInfo(fileUrl.toLocalFile()).fileName() : fileUrl.fileName();
            if (fileName.isEmpty()) {
                fileName = fileUrl.host();
            }
            if (fileName.isEmpty()) {
                fileName = fileUrl.toDisplayString(QUrl::PreferLocalFile);
            }

            const auto line = index.data(JavaScriptConsoleInterface::LineRole).toInt();
            const auto column = index.data(JavaScriptConsoleInterface::ColumnRole).toInt();
            if (line > 0) {
                fileName.append(QLatin1Char(':'));
                fileName.append(QString::number(line));
                if (column > 0) {
                    fileName.append(QLatin1Char(':'));
                    fileName.append(QString::number(column));
                }
            }
            return fileName;
        }

        QString localFilePath(const QUrl &fileUrl) {
            return QFileInfo(fileUrl.toLocalFile()).absoluteFilePath();
        }

        QList<JavaScriptConsoleStackFrame> stackTrace(const QModelIndex &index) {
            return index.data(JavaScriptConsoleInterface::StackTraceRole).value<QList<JavaScriptConsoleStackFrame>>();
        }

        QString stackFrameFileInformation(const JavaScriptConsoleStackFrame &frame) {
            if (frame.fileUrl.isEmpty()) {
                return {};
            }

            auto result = frame.fileUrl.isLocalFile() ? QDir::toNativeSeparators(localFilePath(frame.fileUrl)) : frame.fileUrl.toDisplayString(QUrl::PreferLocalFile);
            if (frame.line > 0) {
                result.append(QLatin1Char(':'));
                result.append(QString::number(frame.line));
            }
            return result;
        }

        JavaScriptConsoleStackFrame messageSource(const QModelIndex &index) {
            const auto fileUrl = index.data(JavaScriptConsoleInterface::FileUrlRole).toUrl();
            if (!fileUrl.isEmpty()) {
                return {
                    {},
                    fileUrl,
                    index.data(JavaScriptConsoleInterface::LineRole).toInt(),
                    index.data(JavaScriptConsoleInterface::ColumnRole).toInt(),
                };
            }
            for (const auto &frame : stackTrace(index)) {
                if (!frame.fileUrl.isEmpty()) {
                    return frame;
                }
            }
            return {};
        }

        QString fullFileInformation(const QModelIndex &index) {
            const auto source = messageSource(index);
            if (source.fileUrl.isEmpty()) {
                return {};
            }

            auto result = source.fileUrl.isLocalFile() ? QDir::toNativeSeparators(localFilePath(source.fileUrl)) : source.fileUrl.toDisplayString(QUrl::PreferLocalFile);
            if (source.line > 0) {
                result.append(QLatin1Char(':'));
                result.append(QString::number(source.line));
                if (source.column > 0) {
                    result.append(QLatin1Char(':'));
                    result.append(QString::number(source.column));
                }
            }
            return result;
        }

        QString levelText(const QModelIndex &index) {
            switch (static_cast<JavaScriptConsoleInterface::Level>(index.data(JavaScriptConsoleInterface::LevelRole).toInt())) {
                case JavaScriptConsoleInterface::Debug:
                    return QStringLiteral("DEBUG");
                case JavaScriptConsoleInterface::Log:
                    return QStringLiteral("LOG");
                case JavaScriptConsoleInterface::Info:
                    return QStringLiteral("INFO");
                case JavaScriptConsoleInterface::Warning:
                    return QStringLiteral("WARNING");
                case JavaScriptConsoleInterface::Error:
                    return QStringLiteral("ERROR");
            }
            return {};
        }

        QString messageText(const QModelIndex &index) {
            auto result = index.data(JavaScriptConsoleInterface::TextRole).toString();
            for (const auto &frame : stackTrace(index)) {
                const auto functionName = frame.functionName.isEmpty() ? QStringLiteral("<anonymous>") : frame.functionName;
                const auto fileText = stackFrameFileInformation(frame);
                result.append(QLatin1Char('\n'));
                result.append(functionName);
                if (!fileText.isEmpty()) {
                    result.append(QLatin1Char(' '));
                    result.append(fileText);
                }
            }
            return result;
        }

        QString formattedMessage(const QModelIndex &index) {
            return QStringLiteral("[%1] [%2] %3").arg(fullFileInformation(index), levelText(index), messageText(index));
        }

        QColor translucentColor(QColor color) {
            color.setAlphaF(color.alphaF() * 0.25);
            return color;
        }

        QPixmap tintedIcon(const QString &name, const QColor &color, qreal devicePixelRatio) {
            const auto pixelExtent = qMax(IconExtent, qRound(IconExtent * devicePixelRatio));
            auto source = SVS::FluentSystemIcons::getIcon(name, SVS::FluentSystemIcons::Auto, IconExtent, SVS::FluentSystemIcons::Filled, SVS::FluentSystemIcons::NoMirror, SVS::FluentSystemIcons::NoRotate, pixelExtent);
            if (source.isNull()) {
                return {};
            }

            QPixmap result(source.size());
            result.fill(Qt::transparent);
            QPainter painter(&result);
            painter.drawPixmap(0, 0, source);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(result.rect(), color);
            painter.end();
            result.setDevicePixelRatio(devicePixelRatio);
            return result;
        }

        class JavaScriptConsoleFilterModel : public QSortFilterProxyModel {
        public:
            explicit JavaScriptConsoleFilterModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {
                m_enabledLevels.fill(true);
            }

            void setFilterText(const QString &filterText) {
                if (m_filterText == filterText) {
                    return;
                }
                m_filterText = filterText;
                invalidateRowsFilter();
            }

            void setLevelEnabled(JavaScriptConsoleInterface::Level level, bool enabled) {
                const auto index = static_cast<std::size_t>(level);
                if (index >= m_enabledLevels.size() || m_enabledLevels[index] == enabled) {
                    return;
                }
                m_enabledLevels[index] = enabled;
                invalidateRowsFilter();
            }

        protected:
            bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
                const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
                const auto level = index.data(JavaScriptConsoleInterface::LevelRole).toInt();
                if (level < JavaScriptConsoleInterface::Debug || level > JavaScriptConsoleInterface::Error || !m_enabledLevels.at(static_cast<std::size_t>(level))) {
                    return false;
                }
                if (m_filterText.isEmpty()) {
                    return true;
                }

                const auto text = index.data(JavaScriptConsoleInterface::TextRole).toString();
                const auto fileUrl = index.data(JavaScriptConsoleInterface::FileUrlRole).toUrl();
                if (text.contains(m_filterText, Qt::CaseInsensitive) ||
                    fileInformation(index).contains(m_filterText, Qt::CaseInsensitive) ||
                    fileUrl.toString(QUrl::FullyEncoded).contains(m_filterText, Qt::CaseInsensitive) ||
                    fileUrl.toDisplayString(QUrl::PreferLocalFile).contains(m_filterText, Qt::CaseInsensitive)) {
                    return true;
                }
                for (const auto &frame : stackTrace(index)) {
                    if (frame.functionName.contains(m_filterText, Qt::CaseInsensitive) ||
                        stackFrameFileInformation(frame).contains(m_filterText, Qt::CaseInsensitive) ||
                        frame.fileUrl.toString(QUrl::FullyEncoded).contains(m_filterText, Qt::CaseInsensitive)) {
                        return true;
                    }
                }
                return false;
            }

        private:
            std::array<bool, 5> m_enabledLevels;
            QString m_filterText;
        };

        class ConsoleOutputDelegate : public QStyledItemDelegate {
        public:
            explicit ConsoleOutputDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {
            }

            void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
                painter->save();
                const auto theme = SVS::Theme::defaultTheme();
                const auto level = static_cast<JavaScriptConsoleInterface::Level>(index.data(JavaScriptConsoleInterface::LevelRole).toInt());

                if (level == JavaScriptConsoleInterface::Info) {
                    painter->fillRect(option.rect, theme->backgroundPrimaryColor());
                } else if (level == JavaScriptConsoleInterface::Warning) {
                    painter->fillRect(option.rect, translucentColor(theme->warningColor()));
                } else if (level == JavaScriptConsoleInterface::Error) {
                    painter->fillRect(option.rect, translucentColor(theme->errorColor()));
                }
                painter->setFont(option.font);
                painter->setRenderHint(QPainter::Antialiasing);
                const auto contentRect = option.rect.adjusted(HorizontalMargin, VerticalMargin, -HorizontalMargin, -VerticalMargin);
                const auto iconRect = QRect(contentRect.left(), contentRect.top() + qMax(0, (option.fontMetrics.height() - IconExtent) / 2), IconExtent, IconExtent);

                QString iconName;
                QColor iconColor;
                switch (level) {
                    case JavaScriptConsoleInterface::Info:
                        iconName = QStringLiteral("info");
                        iconColor = theme->accentColor();
                        break;
                    case JavaScriptConsoleInterface::Warning:
                        iconName = QStringLiteral("warning");
                        iconColor = theme->warningColor();
                        break;
                    case JavaScriptConsoleInterface::Error:
                        iconName = QStringLiteral("error_circle");
                        iconColor = theme->errorColor();
                        break;
                    default:
                        break;
                }
                if (!iconName.isEmpty()) {
                    const auto pixmap = tintedIcon(iconName, iconColor, painter->device()->devicePixelRatioF());
                    painter->drawPixmap(iconRect, pixmap);
                }

                const auto textRect = contentTextRect(option, index);
                const auto document = createDocument(option, index, textRect.width());
                painter->save();
                painter->translate(textRect.topLeft());
                document->drawContents(painter, QRectF(QPointF(), QSizeF(textRect.size())));
                painter->restore();

                const auto linkRect = fileLinkRect(option, index);
                if (linkRect.isValid()) {
                    auto font = option.font;
                    const auto fileUrl = index.data(JavaScriptConsoleInterface::FileUrlRole).toUrl();
                    if (fileUrl.isLocalFile()) {
                        font.setUnderline(true);
                    }
                    painter->setFont(font);
                    painter->setPen(fileUrl.isLocalFile() ? theme->linkColor() : theme->foregroundSecondaryColor());
                    painter->drawText(linkRect, Qt::AlignRight | Qt::AlignTop, fileInformation(index));
                }
                painter->setPen(theme->borderColor());
                painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
                painter->restore();
            }

            QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
                const auto availableWidth = option.rect.width() > 0 ? option.rect.width() : 800;
                auto sizedOption = option;
                sizedOption.rect.setWidth(availableWidth);
                const auto textRect = contentTextRect(sizedOption, index);
                const auto document = createDocument(sizedOption, index, textRect.width());
                const auto contentHeight = std::max({IconExtent, option.fontMetrics.height(), qCeil(document->size().height())});
                return {availableWidth, contentHeight + VerticalMargin * 2};
            }

            QUrl linkAt(const QStyleOptionViewItem &option, const QModelIndex &index, const QPoint &position) const {
                const auto messageFileUrl = index.data(JavaScriptConsoleInterface::FileUrlRole).toUrl();
                if (messageFileUrl.isLocalFile() && fileLinkRect(option, index).contains(position)) {
                    return messageFileUrl;
                }

                const auto textRect = contentTextRect(option, index);
                if (!textRect.contains(position)) {
                    return {};
                }
                const auto document = createDocument(option, index, textRect.width());
                const auto anchor = document->documentLayout()->anchorAt(QPointF(position - textRect.topLeft()));
                const QUrl fileUrl(anchor);
                return fileUrl.isLocalFile() ? fileUrl : QUrl{};
            }

            QUrl firstLocalFileUrl(const QModelIndex &index) const {
                const auto messageFileUrl = index.data(JavaScriptConsoleInterface::FileUrlRole).toUrl();
                if (messageFileUrl.isLocalFile()) {
                    return messageFileUrl;
                }
                for (const auto &frame : stackTrace(index)) {
                    if (frame.fileUrl.isLocalFile()) {
                        return frame.fileUrl;
                    }
                }
                return {};
            }

        private:
            QRect fileLinkRect(const QStyleOptionViewItem &option, const QModelIndex &index) const {
                const auto information = fileInformation(index);
                if (information.isEmpty()) {
                    return {};
                }
                const auto width = option.fontMetrics.horizontalAdvance(information);
                return QRect(option.rect.right() - HorizontalMargin - width + 1, option.rect.top() + VerticalMargin, width, option.fontMetrics.height());
            }

            QRect contentTextRect(const QStyleOptionViewItem &option, const QModelIndex &index) const {
                const auto contentRect = option.rect.adjusted(HorizontalMargin, VerticalMargin, -HorizontalMargin, -VerticalMargin);
                const auto linkRect = fileLinkRect(option, index);
                const auto textLeft = contentRect.left() + IconColumnWidth + ContentSpacing;
                const auto textRight = linkRect.isValid() ? linkRect.left() - ContentSpacing : contentRect.right();
                return QRect(textLeft, contentRect.top(), qMax(1, textRight - textLeft + 1), contentRect.height());
            }

            std::unique_ptr<QTextDocument> createDocument(const QStyleOptionViewItem &option, const QModelIndex &index, int width) const {
                const auto theme = SVS::Theme::defaultTheme();
                const auto primaryColor = theme->foregroundPrimaryColor().name(QColor::HexRgb);
                const auto secondaryColor = theme->foregroundSecondaryColor().name(QColor::HexRgb);
                const auto linkColor = theme->linkColor().name(QColor::HexRgb);

                auto html = QStringLiteral("<span style=\"white-space:pre-wrap\"><span style=\"color:%1\">%2</span>")
                                .arg(primaryColor, escapedText(index.data(JavaScriptConsoleInterface::TextRole).toString()));
                for (const auto &frame : stackTrace(index)) {
                    const auto functionName = frame.functionName.isEmpty() ? QStringLiteral("<anonymous>") : frame.functionName;
                    const auto fileText = stackFrameFileInformation(frame);
                    html.append(QStringLiteral("<br><span style=\"color:%1\">%2</span>").arg(secondaryColor, escapedText(functionName)));
                    if (fileText.isEmpty()) {
                        continue;
                    }
                    html.append(QLatin1Char(' '));
                    if (frame.fileUrl.isLocalFile()) {
                        html.append(QStringLiteral("<a style=\"color:%1;text-decoration:underline\" href=\"%2\">%3</a>")
                                        .arg(linkColor, frame.fileUrl.toString(QUrl::FullyEncoded).toHtmlEscaped(), escapedText(fileText)));
                    } else {
                        html.append(QStringLiteral("<span style=\"color:%1\">%2</span>").arg(secondaryColor, escapedText(fileText)));
                    }
                }
                html.append(QStringLiteral("</span>"));

                auto document = std::make_unique<QTextDocument>();
                document->setDocumentMargin(0);
                document->setDefaultFont(option.font);
                auto textOption = document->defaultTextOption();
                textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
                document->setDefaultTextOption(textOption);
                document->setHtml(html);
                document->setTextWidth(qMax(1, width));
                return document;
            }

            static QString escapedText(QString text) {
                text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
                text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
                text = text.toHtmlEscaped();
                text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
                return text;
            }
        };

        class ConsoleOutputView : public QListView {
        public:
            ConsoleOutputView(QAbstractItemModel *allMessagesModel, QString copyMessageText, QString copyAllMessagesText, QWidget *parent = nullptr)
                : QListView(parent), m_allMessagesModel(allMessagesModel), m_copyMessageText(std::move(copyMessageText)), m_copyAllMessagesText(std::move(copyAllMessagesText)) {
                setMouseTracking(true);
            }

            void setConsoleDelegate(ConsoleOutputDelegate *delegate) {
                m_delegate = delegate;
                setItemDelegate(delegate);
            }

        protected:
            void contextMenuEvent(QContextMenuEvent *event) override {
                const auto messageIndex = indexAt(event->pos());
                QMenu menu(this);
                const auto copyMessageAction = menu.addAction(m_copyMessageText);
                copyMessageAction->setEnabled(messageIndex.isValid());
                const auto copyAllMessagesAction = menu.addAction(m_copyAllMessagesText);
                copyAllMessagesAction->setEnabled(m_allMessagesModel && m_allMessagesModel->rowCount() > 0);

                const auto selectedAction = menu.exec(event->globalPos());
                if (selectedAction == copyMessageAction) {
                    QGuiApplication::clipboard()->setText(formattedMessage(messageIndex));
                } else if (selectedAction == copyAllMessagesAction) {
                    QStringList messages;
                    messages.reserve(m_allMessagesModel->rowCount());
                    for (auto row = 0; row < m_allMessagesModel->rowCount(); ++row) {
                        messages.append(formattedMessage(m_allMessagesModel->index(row, 0)));
                    }
                    QGuiApplication::clipboard()->setText(messages.join(QLatin1Char('\n')));
                }
                event->accept();
            }

            void mouseMoveEvent(QMouseEvent *event) override {
                viewport()->setCursor(linkAt(event->position().toPoint()).isLocalFile() ? Qt::PointingHandCursor : Qt::ArrowCursor);
                QListView::mouseMoveEvent(event);
            }

            void leaveEvent(QEvent *event) override {
                viewport()->unsetCursor();
                QListView::leaveEvent(event);
            }

            void mouseReleaseEvent(QMouseEvent *event) override {
                if (event->button() == Qt::LeftButton) {
                    const auto fileUrl = linkAt(event->position().toPoint());
                    if (fileUrl.isLocalFile()) {
                        SVS::DesktopServices::reveal(localFilePath(fileUrl));
                        event->accept();
                        return;
                    }
                }
                QListView::mouseReleaseEvent(event);
            }

            void keyPressEvent(QKeyEvent *event) override {
                if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
                    const auto fileUrl = m_delegate ? m_delegate->firstLocalFileUrl(currentIndex()) : QUrl{};
                    if (fileUrl.isLocalFile()) {
                        SVS::DesktopServices::reveal(localFilePath(fileUrl));
                        event->accept();
                        return;
                    }
                }
                QListView::keyPressEvent(event);
            }

            bool viewportEvent(QEvent *event) override {
                if (event->type() == QEvent::ToolTip) {
                    const auto helpEvent = static_cast<QHelpEvent *>(event);
                    const auto fileUrl = linkAt(helpEvent->pos());
                    if (fileUrl.isLocalFile()) {
                        QToolTip::showText(helpEvent->globalPos(), QDir::toNativeSeparators(localFilePath(fileUrl)), viewport());
                        return true;
                    }
                    QToolTip::hideText();
                }
                return QListView::viewportEvent(event);
            }

        private:
            QUrl linkAt(const QPoint &position) const {
                const auto index = indexAt(position);
                if (!m_delegate || !index.isValid()) {
                    return {};
                }
                QStyleOptionViewItem option;
                initViewItemOption(&option);
                option.rect = visualRect(index);
                return m_delegate->linkAt(option, index, position);
            }

            ConsoleOutputDelegate *m_delegate{};
            QPointer<QAbstractItemModel> m_allMessagesModel;
            QString m_copyMessageText;
            QString m_copyAllMessagesText;
        };

    }

    JavaScriptDebugConsoleDialog::JavaScriptDebugConsoleDialog(QAbstractItemModel *model, QWidget *parent) : QDialog(parent) {
        setWindowTitle(tr("JavaScript Debug Console"));
        resize(900, 600);

        auto mainLayout = new QVBoxLayout(this);
        auto tabWidget = new QTabWidget(this);
        mainLayout->addWidget(tabWidget);

        auto outputPage = new QWidget(tabWidget);
        auto outputLayout = new QVBoxLayout(outputPage);
        auto toolLayout = new QHBoxLayout;
        outputLayout->addLayout(toolLayout);

        auto clearButton = new QPushButton(tr("&Clear"), outputPage);
        toolLayout->addWidget(clearButton);

        auto filterEdit = new QLineEdit(outputPage);
        filterEdit->setPlaceholderText(tr("Filter Output"));
        filterEdit->setAccessibleName(tr("Filter console output"));
        toolLayout->addWidget(filterEdit, 1);

        auto filterModel = new JavaScriptConsoleFilterModel(this);
        filterModel->setSourceModel(model);
        m_filterModel = filterModel;

        struct LevelButtonDefinition {
            JavaScriptConsoleInterface::Level level;
            const char *text;
        };
        static const LevelButtonDefinition levelButtons[]{
            {JavaScriptConsoleInterface::Error, QT_TR_NOOP("Error")},
            {JavaScriptConsoleInterface::Warning, QT_TR_NOOP("Warning")},
            {JavaScriptConsoleInterface::Info, QT_TR_NOOP("Info")},
            {JavaScriptConsoleInterface::Log, QT_TR_NOOP("Log")},
            {JavaScriptConsoleInterface::Debug, QT_TR_NOOP("Debug")},
        };
        std::array<QToolButton *, 5> levelFilterButtons{};
        std::array<QString, 5> levelNames;
        for (const auto &definition : levelButtons) {
            const auto levelIndex = static_cast<std::size_t>(definition.level);
            const auto levelName = tr(definition.text);
            auto button = new QToolButton(outputPage);
            button->setText(levelName);
            button->setAccessibleName(tr("Show %1 messages").arg(levelName));
            button->setCheckable(true);
            button->setChecked(true);
            button->setToolButtonStyle(Qt::ToolButtonTextOnly);
            toolLayout->addWidget(button);

            levelFilterButtons.at(levelIndex) = button;
            levelNames.at(levelIndex) = levelName;
        }

        const auto levelCounts = std::make_shared<std::array<int, 5>>(std::array<int, 5>{});
        const auto updateLevelCounts = [model, levelCounts](const QModelIndex &parent, int first, int last, int change) {
            for (auto row = first; row <= last; ++row) {
                const auto level = model->index(row, 0, parent).data(JavaScriptConsoleInterface::LevelRole).toInt();
                if (level >= JavaScriptConsoleInterface::Debug && level <= JavaScriptConsoleInterface::Error) {
                    auto &count = levelCounts->at(static_cast<std::size_t>(level));
                    count = qMax(0, count + change);
                }
            }
        };
        updateLevelCounts({}, 0, model->rowCount() - 1, 1);

        const auto updateLevelButtonTexts = [this, levelCounts, levelFilterButtons, levelNames] {
            for (std::size_t index = 0; index < levelFilterButtons.size(); ++index) {
                const auto button = levelFilterButtons.at(index);
                button->setText(button->isChecked() ? levelNames.at(index) : tr("%1 (%L2)").arg(levelNames.at(index)).arg(levelCounts->at(index)));
            }
            processViewUpdates();
        };

        for (const auto &definition : levelButtons) {
            const auto button = levelFilterButtons.at(static_cast<std::size_t>(definition.level));
            connect(button, &QToolButton::toggled, this, [filterModel, updateLevelButtonTexts, level = definition.level](bool checked) {
                filterModel->setLevelEnabled(level, checked);
                updateLevelButtonTexts();
            });
        }
        connect(model, &QAbstractItemModel::rowsInserted, this, [updateLevelCounts, updateLevelButtonTexts](const QModelIndex &parent, int first, int last) {
            updateLevelCounts(parent, first, last, 1);
            updateLevelButtonTexts();
        });
        connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this, [updateLevelCounts](const QModelIndex &parent, int first, int last) {
            updateLevelCounts(parent, first, last, -1);
        });
        connect(model, &QAbstractItemModel::rowsRemoved, this, updateLevelButtonTexts);
        connect(model, &QAbstractItemModel::modelReset, this, [model, levelCounts, updateLevelCounts, updateLevelButtonTexts] {
            levelCounts->fill(0);
            updateLevelCounts({}, 0, model->rowCount() - 1, 1);
            updateLevelButtonTexts();
        });
        auto outputView = new ConsoleOutputView(model, tr("Copy Message"), tr("Copy All Messages"), outputPage);
        outputView->setModel(filterModel);
        outputView->setConsoleDelegate(new ConsoleOutputDelegate(outputView));
        outputView->setSelectionMode(QAbstractItemView::NoSelection);
        outputView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        outputView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        outputView->setResizeMode(QListView::Adjust);
        outputView->setUniformItemSizes(false);
        outputView->setWordWrap(true);
        outputView->viewport()->setAutoFillBackground(true);
        outputLayout->addWidget(outputView, 1);
        m_outputView = outputView;
        updateLevelButtonTexts();

        tabWidget->addTab(outputPage, tr("&Output"));

        connect(clearButton, &QPushButton::clicked, JavaScriptConsoleInterface::instance(), &JavaScriptConsoleInterface::clear);
        connect(filterEdit, &QLineEdit::textChanged, this, [this, filterModel](const QString &text) {
            filterModel->setFilterText(text);
            processViewUpdates();
        });
        connect(model, &QAbstractItemModel::rowsAboutToBeInserted, this, [this] {
            const auto scrollBar = m_outputView->verticalScrollBar();
            m_followOutput = scrollBar->value() >= scrollBar->maximum();
        });
        connect(filterModel, &QAbstractItemModel::rowsInserted, this, [this] {
            if (m_followOutput) {
                m_outputView->scrollToBottom();
            }
            processViewUpdates();
        });
        connect(filterModel, &QAbstractItemModel::rowsRemoved, this, [this] {
            processViewUpdates();
        });
        connect(filterModel, &QAbstractItemModel::modelReset, this, [this] {
            processViewUpdates();
        });

        const auto updateTheme = [this] {
            const auto theme = SVS::Theme::defaultTheme();
            auto palette = m_outputView->palette();
            palette.setColor(QPalette::Base, theme->backgroundPrimaryColor());
            palette.setColor(QPalette::Window, theme->backgroundPrimaryColor());
            palette.setColor(QPalette::Text, theme->foregroundPrimaryColor());
            m_outputView->setPalette(palette);
            m_outputView->setFont(theme->font());
            m_outputView->viewport()->update();
            processViewUpdates();
        };
        const auto theme = SVS::Theme::defaultTheme();
        connect(theme, &SVS::Theme::backgroundPrimaryColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::borderColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::foregroundPrimaryColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::foregroundSecondaryColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::accentColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::warningColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::errorColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::linkColorChanged, this, updateTheme);
        connect(theme, &SVS::Theme::fontChanged, this, updateTheme);
        updateTheme();
    }

    JavaScriptDebugConsoleDialog::~JavaScriptDebugConsoleDialog() = default;

    void JavaScriptDebugConsoleDialog::processViewUpdates() {
        if (!isVisible() || !m_outputView) {
            return;
        }
        m_outputView->viewport()->update();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    void JavaScriptDebugConsoleDialog::showEvent(QShowEvent *event) {
        QDialog::showEvent(event);
        m_outputView->scrollToBottom();
        QTimer::singleShot(0, this, [this] {
            m_outputView->scrollToBottom();
        });
    }

}

#include "moc_JavaScriptDebugConsoleDialog.cpp"
