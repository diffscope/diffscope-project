// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPCONVERSIONWIZARD_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPCONVERSIONWIZARD_H

#include <QDialog>

#include <libresvipformatconverter/internal/LibreSVIPTypes.h>

class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QTabWidget;
class QWindow;

namespace ImportExportManager {
    class FileConverter;
}

namespace LibreSVIPFormatConverter::Internal {

    class JsonSchemaForm;

    class LibreSVIPConversionWizard final : public QDialog {
        Q_OBJECT
    public:
        enum Operation {
            Import,
            Export,
        };

        LibreSVIPConversionWizard(Operation operation, const QString &path, const QByteArray &inputData,
                                  ImportExportManager::FileConverter *owner, QWindow *window);
        ~LibreSVIPConversionWizard() override;

        bool run();
        LibreSVIPPluginInfo selectedExternalPlugin() const;
        LibreSVIPConversionResult conversionResult() const;

    private:
        struct MiddlewarePage {
            LibreSVIPPluginInfo plugin;
            QCheckBox *enabled{};
            JsonSchemaForm *form{};
        };

        QWidget *createExternalPage();
        QWidget *createMiddlewarePage();
        QWidget *createDspxPage();
        QScrollArea *createFormScrollArea() const;
        void rebuildExternalForm(int index);
        void updateAlternativeConverterWarning();
        void updateNavigation();
        bool validateAll(QString *errorMessage) const;
        void executeConversion();

        Operation m_operation;
        QString m_path;
        QByteArray m_inputData;
        ImportExportManager::FileConverter *m_owner{};
        QWindow *m_window{};
        QList<LibreSVIPPluginInfo> m_externalPlugins;
        LibreSVIPPluginInfo m_dspxPlugin;
        QHash<QString, QJsonObject> m_externalValues;
        QList<MiddlewarePage> m_middlewarePages;
        QString m_currentExternalId;
        LibreSVIPConversionResult m_result;

        QListWidget *m_steps{};
        QStackedWidget *m_pages{};
        QComboBox *m_pluginCombo{};
        QLabel *m_alternativeWarning{};
        QScrollArea *m_externalScroll{};
        JsonSchemaForm *m_externalForm{};
        QTabWidget *m_middlewareTabs{};
        JsonSchemaForm *m_dspxForm{};
        QPushButton *m_previousButton{};
        QPushButton *m_nextButton{};
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPCONVERSIONWIZARD_H
