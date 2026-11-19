#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QCache>
#include <QString>
#include <atomic>
#include <mpv/client.h>
#include <mpv/render.h>

namespace kvikplay {

class ThumbnailWorker : public QThread {
    Q_OBJECT

public:
    explicit ThumbnailWorker(QObject* parent = nullptr);
    ~ThumbnailWorker() override;

    void openFile(const QString& filePath);
    void requestThumbnail(double timestamp);
    void stop();

signals:
    void thumbnailReady(double timestamp, const QImage& image);

protected:
    void run() override;

private:
    void initMpv();
    void cleanupMpv();
    bool renderFrame(double timestamp, QImage& outImage);

    mpv_handle* m_mpv{nullptr};
    mpv_render_context* m_rc{nullptr};

    QMutex m_mutex;
    QWaitCondition m_cond;
    std::atomic<bool> m_running{true};

    QString m_pendingFile;
    bool m_hasNewFile{false};

    double m_requestedTime{-1.0};
    bool m_hasRequest{false};
    double m_aspectRatio{16.0 / 9.0};

    QCache<int, QImage> m_cache;
};

} // namespace kvikplay
