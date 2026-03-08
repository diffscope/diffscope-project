#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <QFile>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/BasicModelStrategy.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/WorkspaceInfo.h>

using namespace dspx;

int main(int argc, char **argv) {
    QCoreApplication a(argc, argv);

    QDspx::Model qDspxModel;
    QFile f(":/tst_dspxmodel_dspx/test.dspx");
    f.open(QIODevice::ReadOnly);
    auto data = f.readAll();
    QDspx::SerializationErrorList errors;
    qDspxModel = QDspx::Serializer::deserialize(data, errors);
    qDebug() << errors.size();


    BasicModelStrategy basicModelStrategy;
    Model model(&basicModelStrategy);

    model.fromQDspx(qDspxModel);
    qDebug().noquote() << QDspx::Serializer::serialize(model.toQDspx(), errors);

    qDebug() << model.timeline()->labels()->firstItem()->text();

    auto workspaceInfo = model.createWorkspaceInfo();
    workspaceInfo->setJsonObject({{"test", "test c"}});
    model.workspace()->insertItem("c", workspaceInfo);

    qDebug().noquote() << QDspx::Serializer::serialize(model.toQDspx(), errors);

    // Test TrackList functionality
    qDebug() << "=== Testing TrackList ===";
    
    auto trackList = model.tracks();
    qDebug() << "Initial track count:" << trackList->size();
    
    // Create some tracks
    auto track1 = model.createTrack();
    track1->setName("New Track 1");
    
    auto track2 = model.createTrack();
    track2->setName("New Track 2");
    
    auto track3 = model.createTrack();
    track3->setName("New Track 3");
    
    // Insert tracks
    trackList->insertItem(0, track1);
    trackList->insertItem(1, track2);
    trackList->insertItem(2, track3);
    
    qDebug() << "After insertion, track count:" << trackList->size();
    
    // Print track names
    for (int i = 0; i < trackList->size(); ++i) {
        auto track = trackList->item(i);
        qDebug() << "Track" << i << ":" << track->name();
    }
    
    // Test rotation
    qDebug() << "Testing rotation (0, 1, 3)...";
    trackList->rotate(0, 1, 3);
    
    // Print track names after rotation
    qDebug() << "After rotation:";
    for (int i = 0; i < trackList->size(); ++i) {
        auto track = trackList->item(i);
        qDebug() << "Track" << i << ":" << track->name();
    }
    
    // Test removal
    qDebug() << "Removing track at index 1...";
    auto removedTrack = trackList->removeItem(1);
    qDebug() << "Removed track:" << (removedTrack ? removedTrack->name() : "nullptr");
    qDebug() << "Track count after removal:" << trackList->size();
    
    // Print final track names
    qDebug() << "Final track list:";
    for (int i = 0; i < trackList->size(); ++i) {
        auto track = trackList->item(i);
        qDebug() << "Track" << i << ":" << track->name();
    }
    
    qDebug() << "=== TrackList test complete ===";
    qDebug().noquote() << QDspx::Serializer::serialize(model.toQDspx(), errors);

    return 0;
}