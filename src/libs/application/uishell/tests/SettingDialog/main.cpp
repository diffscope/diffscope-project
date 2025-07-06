#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>

#include <CoreApi/settingcatalog.h>
#include <CoreApi/isettingpage.h>

class TestSettingPage : public Core::ISettingPage {
public:
    TestSettingPage(const QString &id, const QString &title, QObject *parent) : Core::ISettingPage(id, parent) {
        setTitle(title);
        setDescription(title);
    }
    QObject *widget() override {
        return nullptr;
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

    auto settingCatalog = new Core::SettingCatalog(&a);
    auto page1 = new TestSettingPage("test1", "Test 1", &a);
    auto page2 = new TestSettingPage("test2", "Test 2", &a);
    auto page3 = new TestSettingPage("test3", "Test 3", &a);
    page1->addPage(page3);
    settingCatalog->addPage(page1);
    settingCatalog->addPage(page2);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {"settingCatalog", QVariant::fromValue(settingCatalog)}
    });
    engine.load(":/qt/qml/DiffScope/UIShell/Test/SettingDialog/main.qml");

    return a.exec();
}