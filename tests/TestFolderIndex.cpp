#include "TestCheck.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QThread>
#include <QThreadPool>
#include <iostream>
#include "player/FolderIndex.h"

using namespace kvikplay;

static void touch(const QString& path) {
    QFile f(path);
    f.open(QIODevice::WriteOnly);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    const QString d = tmp.path();
    for (const char* n : {"Episode 10.mkv", "Episode 2.mkv", "episode 1.MKV", "notes.txt", "Episode 2.srt"}) {
        touch(d + "/" + n);
    }

    std::cout << "[TestFolderIndex] Natural order, media only..." << std::endl;
    const QStringList list = FolderIndex::listMediaFiles(d);
    CHECK(list.size() == 3);
    CHECK(list[0].endsWith("episode 1.MKV"));
    CHECK(list[1].endsWith("Episode 2.mkv"));
    CHECK(list[2].endsWith("Episode 10.mkv"));

    std::cout << "[TestFolderIndex] Next / previous..." << std::endl;
    FolderIndex index;
    CHECK(index.next(d + "/Episode 2.mkv").endsWith("Episode 10.mkv"));
    CHECK(index.previous(d + "/Episode 2.mkv").endsWith("episode 1.MKV"));
    CHECK(index.previous(d + "/episode 1.MKV").isEmpty());
    CHECK(index.next(d + "/Episode 10.mkv").isEmpty());

    std::cout << "[TestFolderIndex] Cache refreshes when the folder changes..." << std::endl;
    QThread::msleep(20);
    touch(d + "/Episode 11.mkv");
    CHECK(index.next(d + "/Episode 10.mkv").endsWith("Episode 11.mkv"));

    std::cout << "[TestFolderIndex] Prefetch fills the cache in the background..." << std::endl;
    FolderIndex warm;
    warm.prefetch(d);
    QThreadPool::globalInstance()->waitForDone(5000);
    CHECK(warm.files(d).size() == 4);

    std::cout << "[TestFolderIndex] All tests passed!" << std::endl;
    return 0;
}
