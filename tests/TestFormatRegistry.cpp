#include <cassert>
#include <iostream>
#include "formats/FormatRegistry.h"

int main() {
    using namespace kvikplay;
    const auto& reg = FormatRegistry::instance();

    std::cout << "[TestFormatRegistry] Checking formats..." << std::endl;

    // Check common video formats
    assert(reg.isSupported(".mp4"));
    assert(reg.isSupported("video.MKV"));
    assert(reg.isSupported("/path/to/movie.mov"));
    assert(reg.isVideo(".mp4"));
    assert(!reg.isAudio(".mp4"));

    // Check other video formats
    assert(reg.isSupported(".webm"));
    assert(reg.isSupported(".flv"));
    assert(reg.isSupported(".ts"));
    assert(reg.isVideo(".ts"));

    // Check audio formats
    assert(reg.isSupported(".mp3"));
    assert(reg.isSupported(".flac"));
    assert(reg.isSupported("song.WAV"));
    assert(reg.isAudio(".mp3"));
    assert(!reg.isVideo(".mp3"));

    // Check unsupported
    assert(!reg.isSupported(".txt"));
    assert(!reg.isSupported(".exe"));
    assert(!reg.isSupported(""));

    // Check categories
    auto common = reg.formatsByCategory(FormatCategory::CommonVideo);
    assert(!common.empty());
    auto other = reg.formatsByCategory(FormatCategory::OtherVideo);
    assert(!other.empty());
    auto audio = reg.formatsByCategory(FormatCategory::Audio);
    assert(!audio.empty());

    // Check file filter
    QString filter = reg.fileFilterString();
    assert(filter.contains("*.mp4"));
    assert(filter.contains("*.mp3"));

    std::cout << "[TestFormatRegistry] All tests passed!" << std::endl;
    return 0;
}
