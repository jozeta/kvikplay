#include "TestCheck.h"
#include <QCoreApplication>
#include <QImage>
#include <QThread>
#include <QFileInfo>
#include <clocale>
#include <iostream>
#include <set>
#include "player/ThumbnailWorker.h"

using namespace kvikplay;

// Waits for the next thumbnail; returns a null image on timeout.
static QImage nextThumbnail(QCoreApplication& app, ThumbnailWorker& w, double t, int timeoutMs = 8000) {
    QImage result;
    bool got = false;
    auto c = QObject::connect(&w, &ThumbnailWorker::thumbnailReady, [&](double, const QImage& img) {
        result = img;
        got = true;
    });
    w.requestThumbnail(t);
    for (int waited = 0; !got && waited < timeoutMs; waited += 10) {
        app.processEvents();
        QThread::msleep(10);
    }
    QObject::disconnect(c);
    return result;
}

// A real frame of the colour-bar clip has many distinct colours; a blank or
// uninitialised buffer has one or two.
static size_t distinctColours(const QImage& img) {
    std::set<QRgb> colours;
    for (int y = 0; y < img.height(); y += 4)
        for (int x = 0; x < img.width(); x += 4)
            colours.insert(img.pixel(x, y) & 0xF0F0F0);
    return colours.size();
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::setlocale(LC_NUMERIC, "C"); // after Qt, which resets the locale

    ThumbnailWorker worker;

    std::cout << "[TestThumbnails] 16:9 clip gives a real landscape frame..." << std::endl;
    worker.openFile(QFileInfo(QStringLiteral("test_media/test_16_9.mp4")).absoluteFilePath());
    QImage a = nextThumbnail(app, worker, 1.0);
    CHECK(!a.isNull());
    CHECK(a.width() > a.height());
    std::cout << "  " << a.width() << "x" << a.height() << ", colours " << distinctColours(a) << std::endl;
    CHECK(distinctColours(a) >= 6);

    // Format_RGB32 requires 0xFF in the alpha byte. mpv's "bgr0" writes 0 there, and Qt
    // then drew the scaled thumbnail as transparent-but-added: washed out (1.2.0).
    bool opaque = true;
    for (int y = 0; y < a.height() && opaque; ++y) {
        const QRgb* row = reinterpret_cast<const QRgb*>(a.constScanLine(y));
        for (int x = 0; x < a.width(); ++x) {
            if ((row[x] >> 24) != 0xFF) { opaque = false; break; }
        }
    }
    CHECK(opaque);

    std::cout << "[TestThumbnails] 9:16 clip after it gives a portrait frame..." << std::endl;
    worker.openFile(QFileInfo(QStringLiteral("test_media/test_9_16.mov")).absoluteFilePath());
    QImage b = nextThumbnail(app, worker, 1.0);
    CHECK(!b.isNull());
    std::cout << "  " << b.width() << "x" << b.height() << std::endl;
    CHECK(b.height() > b.width());

    std::cout << "[TestThumbnails] Audio-only file gives no thumbnail..." << std::endl;
    worker.openFile(QFileInfo(QStringLiteral("test_media/test_audio.mp3")).absoluteFilePath());
    QImage c = nextThumbnail(app, worker, 1.0, 3000);
    CHECK(c.isNull());

    worker.stop();
    std::cout << "[TestThumbnails] All thumbnail tests passed!" << std::endl;
    return 0;
}
