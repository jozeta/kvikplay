#include "ThumbnailWorker.h"
#include "MpvOptions.h"
#include "formats/FormatRegistry.h"
#include <QDebug>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string_view>

namespace kvikplay {

namespace {
using Clock = std::chrono::steady_clock;

double msSince(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}
} // namespace

ThumbnailWorker::ThumbnailWorker(QObject* parent)
    : QThread(parent)
    , m_cache(120) { // Keep up to 120 thumbnail frames (~6MB total)
    start(QThread::LowPriority); // never compete with the main player for CPU
}

ThumbnailWorker::~ThumbnailWorker() {
    stop();
}

void ThumbnailWorker::openFile(const QString& filePath) {
    QMutexLocker locker(&m_mutex);
    // Only formats Kvikplay plays (same rule as MediaPlayer::openMedia).
    m_pendingFile = FormatRegistry::instance().isSupported(filePath) ? filePath : QString();
    m_hasNewFile = true;
    m_requestedTime = -1.0;
    m_hasRequest = false;
    m_cache.clear();
    // No wake-up: the file is only opened when the first thumbnail is requested,
    // so opening a video never competes with the main player's startup.
}

void ThumbnailWorker::requestThumbnail(double timestamp) {
    int key = static_cast<int>(std::round(timestamp));
    {
        QMutexLocker locker(&m_mutex);
        if (QImage* cached = m_cache.object(key)) {
            QImage imgCopy = *cached;
            locker.unlock();
            emit thumbnailReady(timestamp, imgCopy);
            return;
        }

        m_requestedTime = timestamp;
        m_hasRequest = true;
        m_cond.wakeOne();
    }
}

void ThumbnailWorker::stop() {
    if (!isRunning()) return;
    m_running.store(false);
    m_cond.wakeAll();
    wait();
}

void ThumbnailWorker::initMpv() {
    m_mpv = mpv_create();
    if (!m_mpv) return;

    applyBaseMpvOptions(m_mpv);
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "ao", "null");
    mpv_set_option_string(m_mpv, "audio", "no");
    mpv_set_option_string(m_mpv, "sub", "no");
    mpv_set_option_string(m_mpv, "sub-auto", "no");
    mpv_set_option_string(m_mpv, "pause", "yes");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "hr-seek", "no");
    mpv_set_option_string(m_mpv, "demuxer-readahead-secs", "0");
    mpv_set_option_string(m_mpv, "cache", "no");
    // Hardware decode with copy-back (the software renderer needs frames in RAM):
    // VideoToolbox-copy on macOS, d3d11va-copy on Windows; software if none works.
    mpv_set_option_string(m_mpv, "hwdec", "auto-copy-safe");
    mpv_set_option_string(m_mpv, "vd-lavc-skiploopfilter", "all");
    mpv_set_option_string(m_mpv, "vd-lavc-fast", "yes");
    mpv_set_option_string(m_mpv, "vd-lavc-threads", "2");
    mpv_set_option_string(m_mpv, "sw-fast", "yes");

    if (mpv_initialize(m_mpv) < 0) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_SW)},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&m_rc, m_mpv, params) < 0) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        m_rc = nullptr;
    }
}

void ThumbnailWorker::cleanupMpv() {
    if (m_rc) {
        mpv_render_context_free(m_rc);
        m_rc = nullptr;
    }
    if (m_mpv) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

bool ThumbnailWorker::waitForEvent(mpv_event_id wanted, double timeoutMs) {
    const auto t0 = Clock::now();
    while (m_running.load()) {
        const double left = timeoutMs - msSince(t0);
        if (left <= 0.0) return false;
        mpv_event* ev = mpv_wait_event(m_mpv, std::min(left, 50.0) / 1000.0);
        if (ev->event_id == wanted) return true;
        if (ev->event_id == MPV_EVENT_END_FILE) {
            auto* end = static_cast<mpv_event_end_file*>(ev->data);
            if (end && end->reason == MPV_END_FILE_REASON_ERROR) return false;
        }
        if (ev->event_id == MPV_EVENT_SHUTDOWN) return false;
    }
    return false;
}

bool ThumbnailWorker::waitForFrame(double timeoutMs) {
    const auto t0 = Clock::now();
    while (m_running.load() && msSince(t0) < timeoutMs) {
        if (mpv_render_context_update(m_rc) & MPV_RENDER_UPDATE_FRAME) return true;
        mpv_wait_event(m_mpv, 0.005); // drain events while the frame is prepared
    }
    return false;
}

bool ThumbnailWorker::loadFile(const QString& file) {
    m_fileHasVideo = false;
    m_aspectRatio = 16.0 / 9.0;
    if (!m_mpv || file.isEmpty()) return false;

    QByteArray fileUtf8 = file.toUtf8();
    const char* cmd[] = {"loadfile", fileUtf8.constData(), nullptr};
    if (mpv_command(m_mpv, cmd) < 0) return false;

    // FILE_LOADED: tracks known. PLAYBACK_RESTART: first frame decoded, so
    // video-params (incl. the display aspect) are valid for *this* file.
    if (!waitForEvent(MPV_EVENT_FILE_LOADED, 3000.0)) return false;

    int hasVideo = 0;
    char* vid = mpv_get_property_string(m_mpv, "vid");
    if (vid) {
        hasVideo = (std::string_view(vid) != "no" && std::string_view(vid) != "auto") ? 1 : 0;
        mpv_free(vid);
    }
    if (!hasVideo) return false; // audio-only: never produce thumbnails

    if (!waitForEvent(MPV_EVENT_PLAYBACK_RESTART, 3000.0)) return false;

    double dar = 0.0;
    if (mpv_get_property(m_mpv, "video-params/aspect", MPV_FORMAT_DOUBLE, &dar) >= 0 && dar > 0.05) {
        m_aspectRatio = dar;
    }
    m_fileHasVideo = true;
    return true;
}

bool ThumbnailWorker::renderFrame(double timestamp, QImage& outImage) {
    if (!m_mpv || !m_rc || !m_fileHasVideo) return false;

    QByteArray timeStr = QByteArray::number(timestamp, 'f', 2);
    const char* seekCmd[] = {"seek", timeStr.constData(), "absolute+keyframes", nullptr};
    if (mpv_command(m_mpv, seekCmd) < 0) return false;

    // Only render once the seek has finished and a new frame is ready; otherwise we
    // would grab (and cache) the previous frame or an empty buffer.
    if (!waitForEvent(MPV_EVENT_PLAYBACK_RESTART, 2000.0)) return false;
    if (!waitForFrame(500.0)) return false;

    int w = 160;
    int h = 90;
    if (m_aspectRatio >= (16.0 / 9.0)) {
        h = static_cast<int>(std::round(160.0 / m_aspectRatio));
    } else {
        w = static_cast<int>(std::round(90.0 * m_aspectRatio));
    }
    w = std::clamp(w, 16, 160);
    h = std::clamp(h, 16, 90);
    outImage = QImage(w, h, QImage::Format_RGB32);
    outImage.fill(Qt::black);
    size_t stride = outImage.bytesPerLine();
    int size[2] = {w, h};
    char fmt[] = "bgr0";

    mpv_render_param rparams[] = {
        {MPV_RENDER_PARAM_SW_SIZE, size},
        {MPV_RENDER_PARAM_SW_FORMAT, fmt},
        {MPV_RENDER_PARAM_SW_STRIDE, &stride},
        {MPV_RENDER_PARAM_SW_POINTER, outImage.bits()},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_render(m_rc, rparams) < 0) return false;

    // "bgr0" leaves the 4th byte 0, but Format_RGB32 must be 0xffRRGGBB; with 0 there Qt
    // draws a scaled thumbnail as transparent-yet-added (washed out). Make it opaque.
    for (int y = 0; y < h; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(outImage.scanLine(y));
        for (int x = 0; x < w; ++x) row[x] |= 0xFF000000u;
    }
    return true;
}

void ThumbnailWorker::run() {
    initMpv();

    while (m_running.load()) {
        QString fileToLoad;
        bool loadNeeded = false;
        double timeToRender = -1.0;

        {
            QMutexLocker locker(&m_mutex);
            while (m_running.load() && !m_hasRequest) {
                m_cond.wait(&m_mutex);
            }
            if (!m_running.load()) break;

            if (m_hasNewFile) {
                fileToLoad = m_pendingFile;
                m_hasNewFile = false;
                loadNeeded = true;
            }
            timeToRender = m_requestedTime;
            m_hasRequest = false;
        }

        if (loadNeeded) {
            loadFile(fileToLoad);
        }

        QImage img;
        if (timeToRender >= 0.0 && renderFrame(timeToRender, img)) {
            const int key = static_cast<int>(std::round(timeToRender));
            {
                QMutexLocker locker(&m_mutex);
                if (m_hasNewFile) continue; // a different file was opened meanwhile
                m_cache.insert(key, new QImage(img));
            }
            emit thumbnailReady(timeToRender, img);
        }
    }

    cleanupMpv();
}

} // namespace kvikplay
