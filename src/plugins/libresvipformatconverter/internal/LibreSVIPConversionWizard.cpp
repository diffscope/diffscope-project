#include "LibreSVIPConversionWizard.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QVariant>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <importexportmanager/ConverterCollection.h>
#include <importexportmanager/FileConverter.h>

#include <libresvipformatconverter/internal/JsonSchemaForm.h>
#include <libresvipformatconverter/internal/LibreSVIPManager.h>

namespace LibreSVIPFormatConverter::Internal {

    static QString formatDisplayText(const LibreSVIPPluginInfo &format) {
        QString displayName = format.fileFormat.trimmed();
        if (displayName.isEmpty())
            displayName = format.name.trimmed();
        if (displayName.isEmpty())
            displayName = format.identifier;

        QStringList patterns;
        for (QString suffix : format.suffixes) {
            suffix = suffix.trimmed();
            if (suffix.isEmpty())
                continue;
            const QString pattern = suffix.startsWith(QStringLiteral("*."))
                                        ? suffix
                                        : suffix.startsWith(QLatin1Char('.'))
                                              ? QStringLiteral("*") + suffix
                                              : QStringLiteral("*.%1").arg(suffix);
            if (!patterns.contains(pattern, Qt::CaseInsensitive))
                patterns.append(pattern);
        }
        return patterns.isEmpty()
                   ? displayName
                   : QStringLiteral("%1 (%2)").arg(displayName, patterns.join(QStringLiteral("; ")));
    }

    LibreSVIPConversionWizard::LibreSVIPConversionWizard(Operation operation, const QString &path,
                                                         const QByteArray &inputData,
                                                         ImportExportManager::FileConverter *owner,
                                                         QWindow *window)
        : QDialog(nullptr), m_operation(operation), m_path(path), m_inputData(inputData), m_owner(owner),
          m_window(window) {
        setWindowTitle(operation == Import ? tr("LibreSVIP Import") : tr("LibreSVIP Export"));
        setModal(true);
        if (window) {
            setAttribute(Qt::WA_NativeWindow);
            if (auto *dialogWindow = windowHandle())
                dialogWindow->setTransientParent(window);
        }
        resize(720, 480);
        setMinimumSize(720, 480);

        const auto &catalog = LibreSVIPManager::instance()->catalog();
        m_externalPlugins = operation == Import ? catalog.inputs : catalog.outputs;
        if (const auto *dspx = catalog.find(operation == Import ? LibreSVIPPluginCategory::Output
                                                               : LibreSVIPPluginCategory::Input,
                                                   QStringLiteral("dspx"))) {
            m_dspxPlugin = *dspx;
        }

        auto *rootLayout = new QVBoxLayout(this);
        auto *contentLayout = new QHBoxLayout;
        rootLayout->addLayout(contentLayout, 1);

        m_steps = new QListWidget(this);
        m_steps->setMaximumWidth(220);
        m_steps->setMinimumWidth(170);
        m_steps->addItem(operation == Import ? tr("Import options") : tr("Export options"));
        m_steps->addItem(tr("Middleware options"));
        m_steps->addItem(tr("DSPX options"));
        contentLayout->addWidget(m_steps);

        m_pages = new QStackedWidget(this);
        m_pages->addWidget(createExternalPage());
        m_pages->addWidget(createMiddlewarePage());
        m_pages->addWidget(createDspxPage());
        contentLayout->addWidget(m_pages, 1);

        auto *navigationLayout = new QHBoxLayout;
        rootLayout->addLayout(navigationLayout);
        navigationLayout->addStretch();
        m_previousButton = new QPushButton(tr("Previous"), this);
        m_nextButton = new QPushButton(this);
        auto *cancelButton = new QPushButton(tr("Cancel"), this);
        navigationLayout->addWidget(m_previousButton);
        navigationLayout->addWidget(m_nextButton);
        navigationLayout->addWidget(cancelButton);

        connect(m_steps, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
        connect(m_steps, &QListWidget::currentRowChanged, this, &LibreSVIPConversionWizard::updateNavigation);
        connect(m_previousButton, &QPushButton::clicked, this, [this] {
            m_steps->setCurrentRow(qMax(0, m_steps->currentRow() - 1));
        });
        connect(m_nextButton, &QPushButton::clicked, this, [this] {
            if (m_steps->currentRow() + 1 < m_steps->count())
                m_steps->setCurrentRow(m_steps->currentRow() + 1);
            else
                executeConversion();
        });
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        m_steps->setCurrentRow(0);
        updateNavigation();
    }

    LibreSVIPConversionWizard::~LibreSVIPConversionWizard() = default;

    bool LibreSVIPConversionWizard::run() {
        return exec() == QDialog::Accepted;
    }

    LibreSVIPPluginInfo LibreSVIPConversionWizard::selectedExternalPlugin() const {
        const int index = m_pluginCombo ? m_pluginCombo->currentIndex() : -1;
        return index >= 0 && index < m_externalPlugins.size() ? m_externalPlugins.at(index) : LibreSVIPPluginInfo{};
    }

    LibreSVIPConversionResult LibreSVIPConversionWizard::conversionResult() const {
        return m_result;
    }

    QScrollArea *LibreSVIPConversionWizard::createFormScrollArea() const {
        auto *scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        return scrollArea;
    }

    QWidget *LibreSVIPConversionWizard::createExternalPage() {
        auto *page = new QWidget(this);
        auto *layout = new QVBoxLayout(page);
        auto *selectorLayout = new QHBoxLayout;
        auto *selectorLabel = new QLabel(m_operation == Import ? tr("Import format") : tr("Export format"), page);
        m_pluginCombo = new QComboBox(page);
        selectorLayout->addWidget(selectorLabel);
        selectorLayout->addWidget(m_pluginCombo, 1);
        layout->addLayout(selectorLayout);

        m_alternativeWarning = new QLabel(page);
        m_alternativeWarning->setWordWrap(true);
        m_alternativeWarning->setTextFormat(Qt::PlainText);
        layout->addWidget(m_alternativeWarning);

        m_externalScroll = createFormScrollArea();
        layout->addWidget(m_externalScroll, 1);

        const QString suffix = QFileInfo(m_path).suffix();
        int selectedIndex = 0;
        for (int i = 0; i < m_externalPlugins.size(); ++i) {
            const auto &plugin = m_externalPlugins.at(i);
            m_pluginCombo->addItem(formatDisplayText(plugin), plugin.identifier);
            if (plugin.suffixes.contains(suffix, Qt::CaseInsensitive))
                selectedIndex = i;
        }
        if (!m_externalPlugins.isEmpty())
            m_pluginCombo->setCurrentIndex(selectedIndex);
        connect(m_pluginCombo, &QComboBox::currentIndexChanged, this, &LibreSVIPConversionWizard::rebuildExternalForm);
        rebuildExternalForm(m_pluginCombo->currentIndex());
        return page;
    }

    QWidget *LibreSVIPConversionWizard::createMiddlewarePage() {
        auto *page = new QWidget(this);
        auto *layout = new QVBoxLayout(page);
        m_middlewareTabs = new QTabWidget(page);
        layout->addWidget(m_middlewareTabs);

        for (const auto &plugin : LibreSVIPManager::instance()->catalog().middlewares) {
            auto *tab = new QWidget(m_middlewareTabs);
            auto *tabLayout = new QVBoxLayout(tab);
            auto *enabled = new QCheckBox(tr("Enable this middleware"), tab);
            tabLayout->addWidget(enabled);
            auto *scrollArea = createFormScrollArea();
            auto *form = new JsonSchemaForm(plugin.jsonSchema, scrollArea);
            scrollArea->setWidget(form);
            tabLayout->addWidget(scrollArea, 1);
            form->setEnabled(false);
            connect(enabled, &QCheckBox::toggled, form, &QWidget::setEnabled);
            const int tabIndex = m_middlewareTabs->addTab(tab, plugin.name.isEmpty() ? plugin.identifier : plugin.name);
            m_middlewareTabs->setTabToolTip(tabIndex, plugin.description);
            m_middlewarePages.append({plugin, enabled, form});
        }
        if (m_middlewarePages.isEmpty()) {
            m_middlewareTabs->hide();
            auto *emptyLabel = new QLabel(tr("No middleware is available."), page);
            emptyLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(emptyLabel);
        }
        return page;
    }

    QWidget *LibreSVIPConversionWizard::createDspxPage() {
        auto *page = new QWidget(this);
        auto *layout = new QVBoxLayout(page);
        auto *scrollArea = createFormScrollArea();
        m_dspxForm = new JsonSchemaForm(m_dspxPlugin.jsonSchema, scrollArea);
        scrollArea->setWidget(m_dspxForm);
        layout->addWidget(scrollArea);
        return page;
    }

    void LibreSVIPConversionWizard::rebuildExternalForm(int index) {
        if (m_externalForm && !m_currentExternalId.isEmpty())
            m_externalValues.insert(m_currentExternalId, m_externalForm->value());

        if (auto *oldWidget = m_externalScroll->takeWidget())
            oldWidget->deleteLater();
        m_externalForm = nullptr;
        m_currentExternalId.clear();
        if (index < 0 || index >= m_externalPlugins.size()) {
            updateAlternativeConverterWarning();
            return;
        }

        const auto &plugin = m_externalPlugins.at(index);
        m_currentExternalId = plugin.identifier;
        m_externalForm = new JsonSchemaForm(plugin.jsonSchema, m_externalScroll);
        if (m_externalValues.contains(plugin.identifier))
            m_externalForm->setValue(m_externalValues.value(plugin.identifier));
        m_externalScroll->setWidget(m_externalForm);
        updateAlternativeConverterWarning();
    }

    void LibreSVIPConversionWizard::updateAlternativeConverterWarning() {
        const auto plugin = selectedExternalPlugin();
        QStringList normalPriorityAlternatives;
        QStringList lowPriorityAlternatives;
        for (auto *converter : ImportExportManager::ConverterCollection::fileConverters()) {
            if (!converter || converter == m_owner || converter->mode() != m_owner->mode())
                continue;
            bool supported = false;
            for (const auto &suffix : plugin.suffixes) {
                const QString candidate = QStringLiteral("file.%1").arg(suffix);
                for (const auto &filter : converter->heuristicFilters()) {
                    if (QDir::match(filter, candidate)) {
                        supported = true;
                        break;
                    }
                }
                if (supported)
                    break;
            }
            if (!supported || converter->name().isEmpty())
                continue;
            if (converter->heuristicPriority() == ImportExportManager::FileConverter::Normal)
                normalPriorityAlternatives.append(converter->name());
            else
                lowPriorityAlternatives.append(converter->name());
        }

        QStringList alternatives = normalPriorityAlternatives;
        alternatives.append(lowPriorityAlternatives);
        m_alternativeWarning->setVisible(!alternatives.isEmpty());
        if (alternatives.isEmpty()) {
            m_alternativeWarning->clear();
            return;
        }

        m_alternativeWarning->setText(
            tr("This format is also supported by: %1. Consider using the dedicated converter(s) instead of LibreSVIP.", nullptr, alternatives.size())
                .arg(alternatives.join(tr(" / ", "Separator of alternatives"))));
    }

    void LibreSVIPConversionWizard::updateNavigation() {
        const int row = m_steps->currentRow();
        m_previousButton->setEnabled(row > 0);
        m_nextButton->setText(row == m_steps->count() - 1 ? (m_operation == Import ? tr("Import") : tr("Export")) : tr("Next"));
        m_nextButton->setDefault(row == m_steps->count() - 1);
    }

    bool LibreSVIPConversionWizard::validateAll(QString *errorMessage) const {
        if (!m_externalForm) {
            *errorMessage = m_operation == Import
                                ? tr("No import format is selected.")
                                : tr("No export format is selected.");
            return false;
        }
        QString detail;
        if (!m_externalForm->validate(&detail)) {
            *errorMessage = m_operation == Import
                                ? tr("The selected import format options are invalid: %1").arg(detail)
                                : tr("The selected export format options are invalid: %1").arg(detail);
            return false;
        }
        for (const auto &middleware : m_middlewarePages) {
            if (middleware.enabled->isChecked() && !middleware.form->validate(&detail)) {
                *errorMessage = tr("The options for middleware \"%1\" are invalid: %2")
                                    .arg(middleware.plugin.name, detail);
                return false;
            }
        }
        if (!m_dspxForm->validate(&detail)) {
            *errorMessage = tr("The DSPX options are invalid: %1").arg(detail);
            return false;
        }
        return true;
    }

    void LibreSVIPConversionWizard::executeConversion() {
        QString validationError;
        if (!validateAll(&validationError)) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), m_window,
                                     tr("Invalid LibreSVIP options"), validationError);
            return;
        }

        const auto external = selectedExternalPlugin();
        LibreSVIPConversionRequestData request;
        request.inputData = m_inputData;
        if (m_operation == Import) {
            request.inputIdentifier = external.identifier;
            request.outputIdentifier = m_dspxPlugin.identifier;
            request.inputOptions = m_externalForm->value();
            request.outputOptions = m_dspxForm->value();
        } else {
            request.inputIdentifier = m_dspxPlugin.identifier;
            request.outputIdentifier = external.identifier;
            request.inputOptions = m_dspxForm->value();
            request.outputOptions = m_externalForm->value();
        }
        for (const auto &middleware : m_middlewarePages) {
            if (middleware.enabled->isChecked())
                request.middlewareOptions.insert(middleware.plugin.identifier, middleware.form->value());
        }

        m_result = LibreSVIPManager::instance()->convert(request, m_window);
        if (m_result.cancelled)
            return;
        if (!m_result.success) {
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), m_window,
                                      tr("LibreSVIP conversion failed"), m_result.errorMessage);
            return;
        }
        if (!m_result.warningMessages.isEmpty()) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), m_window,
                                     tr("LibreSVIP conversion warnings"), m_result.warningMessages.join(QStringLiteral("\n")));
        }
        accept();
    }

}
