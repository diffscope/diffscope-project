#include <functional>

#include <QQmlApplicationEngine>
#include <QApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>
#include <QQmlComponent>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

#include <CoreApi/settingcatalog.h>
#include <CoreApi/isettingpage.h>

class TestSettingPage : public Core::ISettingPage {
    Q_OBJECT
    Q_PROPERTY(bool doNotAccept MEMBER m_doNotAccept)
public:
    TestSettingPage(const QString &id, const QString &title, const std::function<QObject *(TestSettingPage *)> &loadWidget, QObject *parent) : Core::ISettingPage(id, parent) {
        setTitle(title);
        setDescription(title + " description");
        m_widget = loadWidget(this);
    }
    QObject *widget() override {
        return m_widget;
    }
    void beginSetting() override {
        qDebug() << "Begin setting:" << id();
        ISettingPage::beginSetting();
    }
    void endSetting() override {
        qDebug() << "End setting:" << id();
        ISettingPage::endSetting();
    }
    bool accept() override {
        qDebug() << "Accept setting:" << id() << !m_doNotAccept;
        if (m_doNotAccept) {
            return false;
        }
        return ISettingPage::accept();
    }

private:
    QObject *m_widget{};
    bool m_doNotAccept = false;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    auto sf = QSurfaceFormat::defaultFormat();
    sf.setSamples(8);
    QSurfaceFormat::setDefaultFormat(sf);

    QQuickStyle::setStyle("SVSCraft.UIComponents");
    QQuickStyle::setFallbackStyle("Basic");

    QQmlApplicationEngine engine;

    auto loadNullPage = [](TestSettingPage *) -> QObject * {
        return nullptr;
    };
    auto loadQuickPage = [&engine](TestSettingPage *page) {
        QQmlComponent component(&engine, ":/qt/qml/DiffScope/UIShell/Test/SettingDialog/SettingPageWidget.qml");
        auto o = component.createWithInitialProperties({{"title", page->title()}, {"iSettingPage", QVariant::fromValue(page)}});
        o->setParent(page);
        return o;
    };
    auto loadWidgetPage = [](TestSettingPage *page) {
        auto widget = new QWidget;
        auto layout = new QVBoxLayout(widget);
        layout->addWidget(new QLabel("QWidget compatible setting page"));
        auto checkBox1 = new QCheckBox(page->title() + " option");
        layout->addWidget(checkBox1);
        auto checkBox2 = new QCheckBox("Do not accept");
        layout->addWidget(checkBox2);
        auto button = new QPushButton(page->title());
        layout->addWidget(button);
        layout->addStretch();
        QObject::connect(checkBox1, &QCheckBox::clicked, page, &TestSettingPage::markDirty);
        QObject::connect(checkBox2, &QCheckBox::clicked, page, [=](bool checked) {
            page->markDirty();
            page->setProperty("doNotAccept", checked);
        });
        return widget;
    };

    auto settingCatalog = new Core::SettingCatalog(&a);
    auto page1 = new TestSettingPage("test1", "Test 1", loadNullPage, &a);
    auto page2 = new TestSettingPage("test2", "Test 2", loadQuickPage, &a);
    auto page3 = new TestSettingPage("test3", "Test 3", loadQuickPage, &a);
    auto page4 = new TestSettingPage("test4", "Test 4", loadQuickPage, &a);
    auto page5 = new TestSettingPage("test5", "Test 5", loadWidgetPage, &a);
    auto page6 = new TestSettingPage("test6", "Test 6", [](auto o) { return new QObject(o); }, &a);
    page1->addPage(page3);
    page1->addPage(page4);
    settingCatalog->addPage(page1);
    settingCatalog->addPage(page2);
    settingCatalog->addPage(page5);
    settingCatalog->addPage(page6);

    engine.setInitialProperties({
        {"settingCatalog", QVariant::fromValue(settingCatalog)}
    });
    engine.load(":/qt/qml/DiffScope/UIShell/Test/SettingDialog/main.qml");

    return a.exec();
}

#include "main.moc"