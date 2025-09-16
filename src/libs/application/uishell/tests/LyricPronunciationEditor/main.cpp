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
            {"jiao", "‰∫?, QStringList{"jiao"}},
            {"zhi", "Áª?, QStringList{"zhi"}},
            {"de", "Áö?, QStringList{"de", "di"}},
            {"luo", "Ëû?, QStringList{"luo"}},
            {"xuan", "Êó?, QStringList{"xuan"}},
            {"xian", "Á∫?, QStringList{"xian"}},
            {"jiao", "‰∫?, QStringList{"jiao"}},
            {"zhi", "Áª?, QStringList{"zhi"}},
            {"zhe", "ÁùÄ", QStringList{"zhe", "zhuo", "zhao"}},
            {"pan", "Áõ?, QStringList{"pan"}},
            {"xuan", "Êó?, QStringList{"xuan"}},
        },
        {
            {"xu", "Âè?, QStringList{"xu"}},
            {"shu", "Ëø?, QStringList{"shu"}},
            {"zhe", "ÁùÄ", QStringList{"zhe", "zhuo", "zhao"}},
            {"ni", "‰Ω?, QStringList{"ni"}},
            {"wo", "Êà?, QStringList{"wo"}},
            {"de", "Áö?, QStringList{"de", "di"}},
            {"cong", "‰ª?, QStringList{"cong"}},
            {"qian", "Ââ?, QStringList{"qian"}},
        },
        {
            {"cong", "‰ª?, QStringList{"cong"}},
            {"qian", "Ââ?, QStringList{"qian"}},
            {"you", "Êú?, QStringList{"you"}},
            {"tai", "Â§?, QStringList{"tai"}},
            {"duo", "Â§?, QStringList{"duo"}},
            {"you", "Êú?, QStringList{"you"}},
            {"tai", "Â§?, QStringList{"tai"}},
            {"duo", "Â§?, QStringList{"duo"}},
            {"gu", "Êï?, QStringList{"gu"}},
            {"shi", "‰∫?, QStringList{"shi"}},
        },
        {
            {"ta", "ÂÆ?, QStringList{"ta"}},
            {"men", "‰ª?, QStringList{"men"}},
            {"ni", "‰Ω?, QStringList{"ni"}},
            {"hai", "Ëø?, QStringList{"hai", "huan"}},
            {"ji", "ËÆ?, QStringList{"ji"}},
            {"de", "Âæ?, QStringList{"de", "dei"}},
            {"ma", "Âê?, QStringList{"ma"}},
        }
    };

    auto model = new QStandardItemModel(lyrics.size(), 1, &a);
    for (int i = 0; i < lyrics.size(); i++) {
        auto item = new QStandardItem;
        for (int j = 0; j < lyrics.at(i).size(); j++) {
            auto rowItem = new QStandardItem;
            rowItem->setData(QVariant::fromValue(lyrics[i][j][0]), USDef::LC_PronunciationRole);
            rowItem->setData(QVariant::fromValue(lyrics[i][j][1]), USDef::LC_LyricRole);
            rowItem->setData(QVariant::fromValue(lyrics[i][j][2]), USDef::LC_CandidatePronunciationsRole);
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
