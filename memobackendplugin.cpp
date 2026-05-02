#include "memobackendplugin.h"

#include "memoblobbackend.h"
#include "memoview.h"

#include "palm/codecs/memocodec.h"
#include "palm/sync/palmbackend.h"
#include "runtime/palmdeviceaccess.h"

#include "conflictrecord.h"   // Kalburator::Sync::QSyncCore::RecordSnapshot

#include <QIcon>
#include <QString>
#include <QWidget>

namespace WildPalms::Memo {

MemoBackendPlugin::MemoBackendPlugin(QObject *parent) : QObject(parent) {}
MemoBackendPlugin::~MemoBackendPlugin() = default;

// --- IPlugin ---

QString MemoBackendPlugin::pluginId()    const { return QStringLiteral("memo"); }
QString MemoBackendPlugin::displayName() const { return QStringLiteral("Memos"); }
QIcon   MemoBackendPlugin::icon()        const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-notes"));
}
QString MemoBackendPlugin::description() const
{
    return QStringLiteral("Synchronizes Palm MemoDB with Markdown files");
}
QString MemoBackendPlugin::version()     const { return QStringLiteral("2.0"); }

// --- IBackendPlugin ---

QStringList MemoBackendPlugin::claimedDatabases() const
{
    return { QStringLiteral("MemoDB") };
}

std::unique_ptr<Kalburator::Sync::IBlobBackend>
MemoBackendPlugin::createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device)
{
    if (!device) return nullptr;
    m_palmBackend = std::make_unique<WildPalms::PalmSync::PalmBackend>(device);
    return std::make_unique<MemoBlobBackend>(m_palmBackend.get(), /*categoryStore=*/nullptr);
}

// --- Main view ---

bool MemoBackendPlugin::hasMainView() const { return true; }

QWidget *MemoBackendPlugin::createMainView(QWidget *parent) const
{
    return new MemoView(parent);
}

QString MemoBackendPlugin::mainViewName() const { return QStringLiteral("Memos"); }

QIcon MemoBackendPlugin::mainViewIcon() const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-notes"));
}

// --- Conflict presentation ---

void MemoBackendPlugin::enrichConflictSnapshot(
    Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
    bool isSourceSide) const
{
    if (snapshot.content.isEmpty()) return;

    if (isSourceSide) {
        const auto decoded = WildPalms::PalmCodecs::decodeMemo(snapshot.content);
        if (decoded.has_value()) {
            snapshot.content = decoded->text.toUtf8();
        }
    }

    QString text = QString::fromUtf8(snapshot.content).trimmed();
    const int nl = text.indexOf(QChar('\n'));
    QString title = (nl > 0) ? text.left(nl).trimmed() : text;
    if (title.length() > 60) title = title.left(57) + QStringLiteral("...");

    snapshot.metadata[QStringLiteral("title")] = title;
    snapshot.contentType = QStringLiteral("text/plain");
}

QString MemoBackendPlugin::formatConflictRecordHtml(
    const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const
{
    QString html;
    const QString title = snapshot.metadata.value(QStringLiteral("title")).toString();
    if (!title.isEmpty()) {
        html += QStringLiteral("<h3>%1</h3>").arg(title.toHtmlEscaped());
    }
    const QString content = QString::fromUtf8(snapshot.content);
    html += QStringLiteral("<pre>%1</pre>").arg(content.toHtmlEscaped());
    return html;
}

} // namespace WildPalms::Memo

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(MemoBackendPluginFactory,
                           "memo-backend-plugin.json",
                           registerPlugin<WildPalms::Memo::MemoBackendPlugin>();)

#include "memobackendplugin.moc"
