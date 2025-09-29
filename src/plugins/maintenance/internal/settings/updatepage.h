#ifndef DIFFSCOPE_MAINTENANCE_UPDATEPAGE_H
#define DIFFSCOPE_MAINTENANCE_UPDATEPAGE_H

#include <QObject>

#include <CoreApi/isettingpage.h>

namespace Maintenance {

    /**
     * @brief Settings page for update configuration
     * 
     * UpdatePage provides a settings interface for configuring application update behavior,
     * including automatic update checking and update channel selection (stable/beta).
     * 
     * This page follows the standard DiffScope settings page pattern:
     * - Properties are synchronized between the QML UI and ApplicationUpdateChecker singleton
     * - beginSetting() loads values from the singleton to the UI
     * - accept() saves values from the UI back to the singleton and persists them
     * 
     * The page is registered under the "General/Updates" category in the settings catalog.
     */
    class UpdatePage : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit UpdatePage(QObject *parent = nullptr);
        ~UpdatePage() override;

        // ISettingPage interface
        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

    private:
        /**
         * @brief Text matching for search functionality in settings
         * @param word The search term to match against
         * @return true if the widget content matches the search term
         */
        bool widgetMatches(const QString &word);
        
        QObject *m_widget{}; ///< QML widget instance for the settings UI
    };

}

#endif //DIFFSCOPE_MAINTENANCE_UPDATEPAGE_H