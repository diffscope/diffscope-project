#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>
#include <QStandardItemModel>

#include <uishell/USDef.h>

using namespace UIShell;

int main(int argc, char *argv[]) {
    QGuiApplication a(argc, argv);

    auto sf = QSurfaceFormat::defaultFormat();
    sf.setSamples(8);
    QSurfaceFormat::setDefaultFormat(sf);

    QQuickStyle::setStyle("SVSCraft.UIComponents");
    QQuickStyle::setFallbackStyle("Basic");

    QList<QList<QVariantList>> lyrics {
        {
            {"jiao", "交", QStringList{"jiao"}},
            {"zhi", "织", QStringList{"zhi"}},
            {"de", "的", QStringList{"de", "di"}},
            {"luo", "螺", QStringList{"luo"}},
            {"xuan", "旋", QStringList{"xuan"}},
            {"xian", "线", QStringList{"xian"}},
            {"jiao", "交", QStringList{"jiao"}},
            {"zhi", "织", QStringList{"zhi"}},
            {"zhe", "着", QStringList{"zhe", "zhuo", "zhao"}},
            {"pan", "盘", QStringList{"pan"}},
            {"xuan", "旋", QStringList{"xuan"}},
        },
        {
            {"xu", "叙", QStringList{"xu"}},
            {"shu", "述", QStringList{"shu"}},
            {"zhe", "着", QStringList{"zhe", "zhuo", "zhao"}},
            {"ni", "你", QStringList{"ni"}},
            {"wo", "我", QStringList{"wo"}},
            {"de", "的", QStringList{"de", "di"}},
            {"cong", "从", QStringList{"cong"}},
            {"qian", "前", QStringList{"qian"}},
        },
        {
            {"cong", "从", QStringList{"cong"}},
            {"qian", "前", QStringList{"qian"}},
            {"you", "有", QStringList{"you"}},
            {"tai", "太", QStringList{"tai"}},
            {"duo", "多", QStringList{"duo"}},
            {"you", "有", QStringList{"you"}},
            {"tai", "太", QStringList{"tai"}},
            {"duo", "多", QStringList{"duo"}},
            {"gu", "故", QStringList{"gu"}},
            {"shi", "事", QStringList{"shi"}},
        },
        {
            {"ta", "它", QStringList{"ta"}},
            {"men", "们", QStringList{"men"}},
            {"ni", "你", QStringList{"ni"}},
            {"hai", "还", QStringList{"hai", "huan"}},
            {"ji", "记", QStringList{"ji"}},
            {"de", "得", QStringList{"de", "dei"}},
            {"ma", "吗", QStringList{"ma"}},
        }
    };

    auto model = new QStandardItemModel(lyrics.size(), 1, &a);
    for (int i = 0; i < lyrics.size(); i++) {
        auto item = new QStandardItem;
        for (int j = 0; j < lyrics.at(i).size(); j++) {
            auto rowItem = new QStandardItem;
            rowItem->setData(QVariant::fromValue(lyrics[i][j][0]), USDef::PronunciationRole);
            rowItem->setData(QVariant::fromValue(lyrics[i][j][1]), USDef::LyricRole);
            rowItem->setData(QVariant::fromValue(lyrics[i][j][2]), USDef::CandidatePronunciationsRole);
            item->appendRow(rowItem);
        }
        model->setItem(i, 0, item);
    }

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {"model", QVariant::fromValue(model)},
    });
    engine.load(":/qt/qml/DiffScope/UIShell/Test/LyricPronunciationEditor/main.qml");

    return a.exec();
}