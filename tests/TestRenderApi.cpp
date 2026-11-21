#include "TestCheck.h"
#include <clocale>
#include <iostream>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

// The bundled libmpv must support the OpenGL render API: Kvikplay draws video through it.
// Without it mpv silently falls back to the "null" video output: sound, but a black
// window (seen on Windows in 1.2.2). No GPU is needed for this check: a build without
// the OpenGL backend answers MPV_ERROR_NOT_IMPLEMENTED before touching OpenGL, while a
// build with it fails later (no real GL here) with a different error.
static void* noProc(void*, const char*) { return nullptr; }

int main() {
    std::setlocale(LC_NUMERIC, "C");
    mpv_handle* mpv = mpv_create();
    CHECK(mpv != nullptr);
    mpv_set_option_string(mpv, "vo", "libmpv");
    CHECK(mpv_initialize(mpv) >= 0);

    mpv_opengl_init_params gl{noProc, nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    mpv_render_context* rc = nullptr;
    const int err = mpv_render_context_create(&rc, mpv, params);
    std::cout << "[TestRenderApi] mpv_render_context_create without GL: " << err << " ("
              << mpv_error_string(err) << ")" << std::endl;
    if (rc) mpv_render_context_free(rc);
    mpv_terminate_destroy(mpv);

    CHECK(err != MPV_ERROR_NOT_IMPLEMENTED); // OpenGL render backend is compiled in
    std::cout << "[TestRenderApi] All tests passed!" << std::endl;
    return 0;
}
