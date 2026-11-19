#include "VideoRenderer.h"
#include <QPainter>
#include <QLinearGradient>
#include <QSvgRenderer>
#include <QDebug>

namespace kvikplay {

VideoRenderer::VideoRenderer(QWidget* parent)
    : QOpenGLWidget(parent) {
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

VideoRenderer::~VideoRenderer() {
    if (isValid()) {
        makeCurrent();
        if (m_mpv_gl) {
            mpv_render_context_set_update_callback(m_mpv_gl, nullptr, nullptr);
            mpv_render_context_free(m_mpv_gl);
            m_mpv_gl = nullptr;
        }
        doneCurrent();
    }
}

void VideoRenderer::setMpvHandle(mpv_handle* mpv) {
    m_mpv = mpv;
}

void VideoRenderer::setMediaInfo(const MediaInfo& info) {
    m_mediaInfo = info;
    m_hasVideo = info.hasVideo();
    update();
}

void* VideoRenderer::getProcAddress(void* ctx, const char* name) {
    Q_UNUSED(ctx);
    QOpenGLContext* glctx = QOpenGLContext::currentContext();
    if (!glctx) return nullptr;
    return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
}

void VideoRenderer::onRenderUpdateStatic(void* ctx) {
    auto* self = static_cast<VideoRenderer*>(ctx);
    QMetaObject::invokeMethod(self, "requestRenderUpdate", Qt::QueuedConnection);
}

void VideoRenderer::requestRenderUpdate() {
    if (!m_mpv_gl) return;
    uint64_t flags = mpv_render_context_update(m_mpv_gl);
    if (flags & MPV_RENDER_UPDATE_FRAME) {
        update();
    }
}

void VideoRenderer::initializeGL() {
    if (m_initialized || !m_mpv) return;

    mpv_opengl_init_params gl_init_params{
        .get_proc_address = getProcAddress,
        .get_proc_address_ctx = nullptr
    };

    int advanced_control = 0;
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced_control},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&m_mpv_gl, m_mpv, params) < 0) {
        qWarning() << "Failed to initialize mpv OpenGL render context";
        return;
    }

    mpv_render_context_set_update_callback(m_mpv_gl, onRenderUpdateStatic, this);
    m_initialized = true;
}

void VideoRenderer::resizeGL(int w, int h) {
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void VideoRenderer::paintGL() {
    if (m_hasVideo && m_mpv_gl) {
        GLint fboId = defaultFramebufferObject();
        qreal dpr = devicePixelRatio();
        int w = static_cast<int>(width() * dpr);
        int h = static_cast<int>(height() * dpr);

        mpv_opengl_fbo fbo{
            .fbo = fboId,
            .w = w,
            .h = h,
            .internal_format = 0
        };

        int flip_y = 1; // Qt FBO coordinate orientation
        mpv_render_param params[]{
            {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        mpv_render_context_render(m_mpv_gl, params);
    } else {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        drawPlaceholder(painter);
    }
}

void VideoRenderer::drawPlaceholder(QPainter& painter) {
    // Deep Nordic Slate minimalist background
    QLinearGradient bgGrad(0, 0, width(), height());
    bgGrad.setColorAt(0.0, QColor(0x18, 0x1a, 0x20));
    bgGrad.setColorAt(1.0, QColor(0x0e, 0x10, 0x13));
    painter.fillRect(rect(), bgGrad);

    if (m_mediaInfo.isAudioOnly) {
        // Draw elegant audio visualization disc
        QSvgRenderer discRenderer(QStringLiteral(":/icons/audio_disc.svg"));
        int discSize = std::min(width(), height()) / 3;
        discSize = std::clamp(discSize, 64, 160);
        QRect discRect(width() / 2 - discSize / 2, height() / 2 - discSize / 2 - 20, discSize, discSize);

        painter.setOpacity(0.85);
        discRenderer.render(&painter, discRect);

        // Display track title
        painter.setPen(QColor(0xee, 0xee, 0xee));
        QFont font = painter.font();
        font.setPointSize(14);
        font.setWeight(QFont::Medium);
        painter.setFont(font);

        QString title = m_mediaInfo.title.isEmpty() ? QStringLiteral("Audio Track") : m_mediaInfo.title;
        QRect textRect(20, discRect.bottom() + 16, width() - 40, 30);
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextSingleLine, title);
    } else {
        // Neutral idle state
        QSvgRenderer appIcon(QStringLiteral(":/icons/app_icon.svg"));
        int iconSize = std::min(width(), height()) / 4;
        iconSize = std::clamp(iconSize, 48, 120);
        QRect iconRect(width() / 2 - iconSize / 2, height() / 2 - iconSize / 2 - 12, iconSize, iconSize);

        painter.setOpacity(0.35);
        appIcon.render(&painter, iconRect);

        painter.setOpacity(0.4);
        painter.setPen(QColor(0xaa, 0xaa, 0xaa));
        QFont font = painter.font();
        font.setPointSize(12);
        painter.setFont(font);

        QRect textRect(20, iconRect.bottom() + 16, width() - 40, 24);
        painter.drawText(textRect, Qt::AlignCenter, QStringLiteral("Drop media here or right-click to open"));
    }
}

} // namespace kvikplay
