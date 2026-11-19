#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QDir>
#include <iostream>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    if (argc < 3) {
        std::cerr << "Usage: generate_icon <input.svg> <output_iconset_dir>" << std::endl;
        return 1;
    }

    QString svgPath = QString::fromUtf8(argv[1]);
    QString iconsetDir = QString::fromUtf8(argv[2]);
    QDir().mkpath(iconsetDir);

    QSvgRenderer renderer(svgPath);
    if (!renderer.isValid()) {
        std::cerr << "Error: Invalid SVG file: " << svgPath.toStdString() << std::endl;
        return 1;
    }

    struct SizeInfo {
        const char* name;
        int size;
    };

    SizeInfo sizes[] = {
        {"icon_16x16.png", 16},
        {"icon_16x16@2x.png", 32},
        {"icon_32x32.png", 32},
        {"icon_32x32@2x.png", 64},
        {"icon_128x128.png", 128},
        {"icon_128x128@2x.png", 256},
        {"icon_256x256.png", 256},
        {"icon_256x256@2x.png", 512},
        {"icon_512x512.png", 512},
        {"icon_512x512@2x.png", 1024}
    };

    for (const auto& s : sizes) {
        QImage img(s.size, s.size, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter);
        painter.end();

        QString outPath = QString("%1/%2").arg(iconsetDir, s.name);
        if (!img.save(outPath, "PNG")) {
            std::cerr << "Failed to save: " << outPath.toStdString() << std::endl;
            return 1;
        }
        std::cout << "Rendered " << s.name << " (" << s.size << "x" << s.size << ")" << std::endl;
    }

    std::cout << "Successfully generated all iconset PNGs in " << iconsetDir.toStdString() << std::endl;
    return 0;
}
