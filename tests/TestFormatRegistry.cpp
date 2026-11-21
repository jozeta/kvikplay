#include "TestCheck.h"
#include <iostream>
#include "formats/FormatRegistry.h"

int main() {
    using namespace kvikplay;
    const auto& reg = FormatRegistry::instance();

    std::cout << "[TestFormatRegistry] Checking formats..." << std::endl;

    // Check common video formats
    CHECK(reg.isSupported(".mp4"));
    CHECK(reg.isSupported("video.MKV"));
    CHECK(reg.isSupported("/path/to/movie.mov"));
    CHECK(reg.isVideo(".mp4"));
    CHECK(!reg.isAudio(".mp4"));

    // Check other video formats
    CHECK(reg.isSupported(".webm"));
    CHECK(reg.isSupported(".flv"));
    CHECK(reg.isSupported(".ts"));
    CHECK(reg.isVideo(".ts"));

    // Check audio formats
    CHECK(reg.isSupported(".mp3"));
    CHECK(reg.isSupported(".flac"));
    CHECK(reg.isSupported("song.WAV"));
    CHECK(reg.isAudio(".mp3"));
    CHECK(!reg.isVideo(".mp3"));

    // Check unsupported
    CHECK(!reg.isSupported(".txt"));
    CHECK(!reg.isSupported(".exe"));
    CHECK(!reg.isSupported(""));

    // Check categories
    auto common = reg.formatsByCategory(FormatCategory::CommonVideo);
    CHECK(!common.empty());
    auto other = reg.formatsByCategory(FormatCategory::OtherVideo);
    CHECK(!other.empty());
    auto audio = reg.formatsByCategory(FormatCategory::Audio);
    CHECK(!audio.empty());

    // Check file filter
    QString filter = reg.fileFilterString();
    CHECK(filter.contains("*.mp4"));
    CHECK(filter.contains("*.mp3"));

    std::cout << "[TestFormatRegistry] All tests passed!" << std::endl;
    return 0;
}
