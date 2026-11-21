#include "TestCheck.h"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QThread>
#include <iostream>
#include "ui/PlaybackOverlay.h"
#include "ui/TimelinePreviewCard.h"

using namespace kvikplay;

// Renders the overlay offscreen over a background and checks what it draws.
//   KVIKPLAY_RENDER_BG   optional background image (e.g. a video frame)
//   KVIKPLAY_RENDER_OUT  optional folder: the rendered states are saved there as PNG
static void pump(int ms) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        QApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(10);
    }
}

static double meanLuma(const QImage& img, const QRect& r) {
    double sum = 0;
    int n = 0;
    for (int y = r.top(); y <= r.bottom(); y += 4)
        for (int x = r.left(); x <= r.right(); x += 4) {
            sum += qGray(img.pixel(x, y));
            ++n;
        }
    return n ? sum / n : 0;
}

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    const QString out = qEnvironmentVariable("KVIKPLAY_RENDER_OUT");

    QImage bg(qEnvironmentVariable("KVIKPLAY_RENDER_BG"));
    if (bg.isNull()) { // CI: a bright generated picture (worst case for white controls)
        bg = QImage(1280, 720, QImage::Format_RGB32);
        QPainter p(&bg);
        QLinearGradient g(0, 0, 1280, 720);
        g.setColorAt(0, QColor(250, 235, 200));
        g.setColorAt(1, QColor(170, 220, 160));
        p.fillRect(bg.rect(), g);
    }
    bg = bg.scaled(1280, 720, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QWidget host;
    host.resize(1280, 720);
    auto* picture = new QLabel(&host);
    picture->setPixmap(QPixmap::fromImage(bg));
    picture->setGeometry(0, 0, 1280, 720);
    auto* overlay = new PlaybackOverlay(&host);
    overlay->setGeometry(0, 0, 1280, 720);
    host.show();
    pump(100);
    const QImage plain = host.grab().toImage();

    std::cout << "[TestOverlayRender] Controls over the picture..." << std::endl;
    ChapterInfo c1; c1.title = QStringLiteral("Opening"); c1.time = 0;
    ChapterInfo c2; c2.title = QStringLiteral("The Chase"); c2.time = 236;
    overlay->setTitle(QStringLiteral("Big Buck Bunny"), QStringLiteral("big_buck_bunny_1080p_h264.mov"));
    overlay->setDuration(596.0);
    overlay->setChapters({c1, c2});
    overlay->setPlaying(true);
    overlay->setVolume(75);
    overlay->showOverlay();
    overlay->setPosition(95.0);
    pump(400); // fade-in
    const QImage shown = host.grab().toImage();
    if (!out.isEmpty()) shown.save(out + QStringLiteral("/osd-controls.png"));

    // The bottom scrim darkens the picture behind the controls.
    const QRect bottomStrip(0, 700, 1280, 16);
    std::cout << "  bottom luma " << meanLuma(plain, bottomStrip) << " -> " << meanLuma(shown, bottomStrip) << std::endl;
    CHECK(meanLuma(shown, bottomStrip) < meanLuma(plain, bottomStrip) - 40);

    // Times at both ends of the timeline.
    QStringList texts;
    for (QLabel* l : overlay->findChildren<QLabel*>()) texts << l->text();
    for (QPushButton* b : overlay->findChildren<QPushButton*>()) texts << b->text();
    CHECK(texts.contains(QStringLiteral("1:35")));
    CHECK(texts.contains(QStringLiteral("−8:21")));
    CHECK(texts.contains(QStringLiteral("Big Buck Bunny")));

    // The play/pause glyph is drawn (white pixels where the button sits).
    QPushButton* playPause = nullptr;
    for (QPushButton* b : overlay->findChildren<QPushButton*>())
        if (b->accessibleName() == QStringLiteral("Pause")) playPause = b;
    CHECK(playPause != nullptr);
    const QRect glyph = playPause->geometry().adjusted(14, 14, -14, -14);
    CHECK(meanLuma(shown, glyph) > meanLuma(shown, glyph.translated(400, 0)) + 20); // vs. empty scrim in the same row

    std::cout << "[TestOverlayRender] Volume ring in the centre..." << std::endl;
    overlay->showVolumeLevel(75, false);
    pump(300);
    const QImage volume = host.grab().toImage();
    if (!out.isEmpty()) volume.save(out + QStringLiteral("/osd-volume.png"));
    const QRect centre(560, 260, 160, 140);
    CHECK(meanLuma(volume, centre) != meanLuma(shown, centre)); // something was drawn there

    std::cout << "[TestOverlayRender] Hover preview..." << std::endl;
    auto* card = overlay->findChild<TimelinePreviewCard*>();
    CHECK(card != nullptr);
    QImage thumb(qEnvironmentVariable("KVIKPLAY_RENDER_THUMB"));
    if (thumb.isNull()) {
        thumb = QImage(160, 90, QImage::Format_RGB32);
        thumb.fill(QColor(90, 140, 200));
    }
    card->setTime(227.0, QStringLiteral("The Chase"));
    card->setThumbnail(thumb);
    card->updatePosition(500, 620, 1280);
    card->show();
    card->raise();
    pump(100);
    const QImage preview = host.grab().toImage();
    if (!out.isEmpty()) preview.save(out + QStringLiteral("/osd-preview.png"));
    CHECK(meanLuma(preview, card->geometry()) != meanLuma(shown, card->geometry()));

    std::cout << "[TestOverlayRender] All tests passed!" << std::endl;
    return 0;
}
