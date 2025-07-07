#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>
#include <QQmlComponent>

#include <CoreApi/settingcatalog.h>
#include <CoreApi/isettingpage.h>

class TestSettingPage : public Core::ISettingPage {
    QObject *m_widget{};
public:
    TestSettingPage(const QString &id, const QString &title, QQmlEngine *engine, bool loadWidget, QObject *parent) : Core::ISettingPage(id, parent) {
        setTitle(title);
        setDescription(title + " description");
        if (loadWidget) {
            QQmlComponent component(engine, ":/qt/qml/DiffScope/UIShell/Test/SettingDialog/SettingPageWidget.qml");
            m_widget = component.createWithInitialProperties({{"title", title}});
            m_widget->setParent(this);
        }
    }
    QObject *widget() override {
        return m_widget;
    }
    bool accept() override {
        return true;
    }
};

int main(int argc, char *argv[]) {
    QGuiApplication a(argc, argv);

    auto sf = QSurfaceFormat::defaultFormat();
    sf.setSamples(8);
    QSurfaceFormat::setDefaultFormat(sf);

    QQuickStyle::setStyle("SVSCraft.UIComponents");
    QQuickStyle::setFallbackStyle("Basic");

    QQmlApplicationEngine engine;

    auto settingCatalog = new Core::SettingCatalog(&a);
    auto page1 = new TestSettingPage("test1", "Test 1", &engine, false, &a);
    auto page2 = new TestSettingPage("test2", "Test 2", &engine, true, &a);
    auto page3 = new TestSettingPage("test3", "Test 3", &engine, true, &a);
    auto page4 = new TestSettingPage("test4", "Test 4", &engine, true, &a);
    page1->addPage(page3);
    page1->addPage(page4);
    settingCatalog->addPage(page1);
    settingCatalog->addPage(page2);

    engine.setInitialProperties({
        {"settingCatalog", QVariant::fromValue(settingCatalog)}
    });
    engine.load(":/qt/qml/DiffScope/UIShell/Test/SettingDialog/main.qml");

    return a.exec();
}