#include "ThumbnailWorker.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace kvikplay {

ThumbnailWorker::ThumbnailWorker(QObject* parent)
    : QThread(parent)
    , m_cache(120) { // Keep up to 120 thumbnail frames (~6MB total)
    start();
}

ThumbnailWorker::~ThumbnailWorker() {
    stop();
}

void ThumbnailWorker::openFile(const QString& filePath) {
    QMutexLocker locker(&m_mutex);
    m_pendingFile = filePath;
    m_hasNewFile = true;
    m_requestedTime = -1.0;
    m_hasRequest = false;
    m_cond.wakeOne();
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

    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "ao", "null");
    mpv_set_option_string(m_mpv, "audio", "no");
    mpv_set_option_string(m_mpv, "sub", "no");
    mpv_set_option_string(m_mpv, "pause", "yes");
    mpv_set_option_string(m_mpv, "hr-seek", "no");
    mpv_set_option_string(m_mpv, "demuxer-readahead-secs", "0");
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

bool ThumbnailWorker::renderFrame(double timestamp, QImage& outImage) {
    if (!m_mpv || !m_rc) return false;

    QByteArray timeStr = QByteArray::number(timestamp, 'f', 2);
    const char* seekCmd[] = {"seek", timeStr.constData(), "absolute+keyframes", nullptr};
    mpv_command(m_mpv, seekCmd);

    for (int i = 0; i < 15 && m_running.load(); ++i) {
        mpv_event* ev = mpv_wait_event(m_mpv, 0.01);
        if (ev->event_id == MPV_EVENT_NONE) {
            uint64_t flags = mpv_render_context_update(m_rc);
            if (flags & MPV_RENDER_UPDATE_FRAME) break;
        }
    }

    int w = 160;
    int h = 90;
    if (m_aspectRatio >= (16.0 / 9.0)) {
        w = 160;
        h = static_cast<int>(std::round(160.0 / m_aspectRatio));
    } else if (m_aspectRatio > 0.05) {
        h = 90;
        w = static_cast<int>(std::round(90.0 * m_aspectRatio));
    }
    w = std::clamp(w, 32, 160);
    h = std::clamp(h, 32, 90);
    outImage = QImage(w, h, QImage::Format_RGB32);
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

    int err = mpv_render_context_render(m_rc, rparams);
    return (err >= 0);
}

void ThumbnailWorker::run() {
    initMpv();

    while (m_running.load()) {
        QString fileToLoad;
        bool loadNeeded = false;
        double timeToRender = -1.0;
        bool renderNeeded = false;

        {
            QMutexLocker locker(&m_mutex);
            while (m_running.load() && !m_hasNewFile && !m_hasRequest) {
                m_cond.wait(&m_mutex);
            }
            if (!m_running.load()) break;

            if (m_hasNewFile) {
                fileToLoad = m_pendingFile;
                m_hasNewFile = false;
                loadNeeded = true;
                m_cache.clear();
            }
            if (m_hasRequest) {
                timeToRender = m_requestedTime;
                m_hasRequest = false;
                renderNeeded = true;
            }
        }

        if (loadNeeded && m_mpv && !fileToLoad.isEmpty()) {
            QByteArray fileUtf8 = fileToLoad.toUtf8();
            const char* cmd[] = {"loadfile", fileUtf8.constData(), nullptr};
            mpv_command(m_mpv, cmd);

            for (int i = 0; i < 60 && m_running.load(); ++i) {
                mpv_event* ev = mpv_wait_event(m_mpv, 0.05);
                if (ev->event_id == MPV_EVENT_FILE_LOADED) break;
            }

            double dar = 0.0;
            if (mpv_get_property(m_mpv, "video-params/aspect", MPV_FORMAT_DOUBLE, &dar) >= 0 && dar > 0.05) {
                m_aspectRatio = dar;
            } else {
                int64_t vidW = 0, vidH = 0;
                mpv_get_property(m_mpv, "video-params/w", MPV_FORMAT_INT64, &vidW);
                mpv_get_property(m_mpv, "video-params/h", MPV_FORMAT_INT64, &vidH);
                if (vidW > 0 && vidH > 0) {
                    m_aspectRatio = static_cast<double>(vidW) / vidH;
                }
            }
        }

        if (renderNeeded && m_rc && timeToRender >= 0.0) {
            QImage img;
            if (renderFrame(timeToRender, img)) {
                int key = static_cast<int>(std::round(timeToRender));
                {
                    QMutexLocker locker(&m_mutex);
                    m_cache.insert(key, new QImage(img));
                }
                emit thumbnailReady(timeToRender, img);
            }
        }
    }

    cleanupMpv();
}

} // namespace kvikplay
