#pragma once

#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <atomic>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include "player/MediaInfo.h"

namespace kvikplay {

class VideoRenderer : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit VideoRenderer(QWidget* parent = nullptr);
    ~VideoRenderer() override;

    void setMpvHandle(mpv_handle* mpv);
    void setMediaInfo(const MediaInfo& info);
    bool hasVideo() const { return m_hasVideo; }

public slots:
    void requestRenderUpdate();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    static void* getProcAddress(void* ctx, const char* name);
    static void onRenderUpdateStatic(void* ctx);
    void drawPlaceholder(QPainter& painter);

    mpv_handle* m_mpv{nullptr};
    mpv_render_context* m_mpv_gl{nullptr};
    bool m_hasVideo{false};
    MediaInfo m_mediaInfo;
    bool m_initialized{false};
    std::atomic<bool> m_renderUpdatePending{false};
};

} // namespace kvikplay
