#include <QtWebEngineWidgets/QWebEngineView>
#include <QApplication>
#include <QLoggingCategory>
#include <QDir>
#include <QStringLiteral>

#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineProfile>
#include <QWebChannel>
#include <QDebug>
#include <QImage>
#include <QBuffer>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>

#include <QMainWindow>
#include <QToolBar>
#include <QFileDialog>

#include "deepzoom.hpp"

#include <memory>
#include <sstream>

// tiles viewer based on `OpenSeaDragon`, displayed on `QwebEngineView`
// use `QuPath` + `bioformats` to support more WSI formats

class M_WebEnginePage: public QWebEnginePage
{
protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber,
                                  const QString& sourceID) override
    {
        qDebug() << Q_FUNC_INFO << ": " << level << message << lineNumber << sourceID;
    }
};

class SlideHandler: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mpp MEMBER m_mpp NOTIFY mppChanged)
    Q_PROPERTY(QString dzi MEMBER m_dzi NOTIFY dziChanged)
public:
    SlideHandler(QObject* p = nullptr): QObject(p) {}
    ~SlideHandler() = default;

    void setSlide(QString url, int tile_size, int overlap)
    {
        m_slide_handler = std::make_unique<DeepZoomGenerator>(url.toStdString(), tile_size, overlap);
        m_dzi = QString::fromStdString(m_slide_handler->get_dzi());
        emit dziChanged(m_dzi);
        m_mpp = QString::number(m_slide_handler->get_mpp());
        emit mppChanged(m_mpp);
    }

public slots:
    QString getTileUrl(QString url)
    {
        // qDebug() << Q_FUNC_INFO << ": " << url;
        auto lxy = url.split("/");
        auto level = lxy[0].toInt();
        auto xy = lxy[1].split("_");
        auto x = xy[0].toInt();
        auto y = xy[1].toInt();

        auto bytes = m_slide_handler->get_tile(level, x, y);
        return "data:image/png;base64," + QByteArray(reinterpret_cast<char*>(bytes.data()), bytes.size()).toBase64();
    }
signals:
    void mppChanged(QString mpp);
    void dziChanged(QString dzi);

private:
    std::unique_ptr<DeepZoomGenerator> m_slide_handler = nullptr;
    QString m_mpp = "1e-6";
    QString m_dzi;
};

int main(int argc, char* argv[])
{
    using namespace Qt::StringLiterals;

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--enable-logging --log-level=0");
    qputenv("QTWEBENGINE_LOCALES_PATH", "./translations/Qt6/qtwebengine_locales");
    QLoggingCategory::setFilterRules(u"qt.webenginecontext.debug=true"_s);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("Tiles Viewer"));

    auto page = QSharedPointer<M_WebEnginePage>::create();
    auto view = QSharedPointer<QWebEngineView>::create();
    view->setPage(page.get());

    auto channel = QSharedPointer<QWebChannel>::create();
    page->setWebChannel(channel.get());

    auto slide_handler = QSharedPointer<SlideHandler>::create(view.get());
    channel->registerObject("slide_handler", slide_handler.get());

    QString localHtmlPath = QDir::currentPath() + "/tiles_viewer/demo.html";
    view->setUrl(QUrl::fromLocalFile(localHtmlPath));

    auto file_toolbar = window.addToolBar("File");
    file_toolbar->addAction("Open", [&]() {
        auto url = QFileDialog::getOpenFileName(&window, "Open Images", QDir::homePath());
        if (url.isEmpty()) return;
        slide_handler->setSlide(url, 254, 0);
    });

    window.setCentralWidget(view.get());
    window.resize(1024, 768);
    window.show();

    return app.exec();
}

#include "tilesviewer.moc"