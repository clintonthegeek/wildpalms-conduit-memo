#include "memobackendplugin.h"

#include "hubmemoreader.h"
#include "memoblobbackend.h"
#include "memoview.h"
#include "notedomainextension.h"

#include "palm/calendar/categoryappinforeader.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/codecs/memocodec.h"
#include "palm/sync/palmbackend.h"
#include "runtime/palmdeviceaccess.h"
#include "runtime/palmruntime.h"

#include "conflictrecord.h"   // Kalburator::Conflict::RecordSnapshot

#include <QIcon>
#include <QString>
#include <QWidget>

namespace WildPalms::Memo {

MemoPlugin::MemoPlugin()
    : m_categoryStore(
        std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
{
    // O7: shape registration moved out of the ctor into shapeContributions();
    // PluginManager registers the contribution into the injected ShapeRegistries.
}
MemoPlugin::~MemoPlugin() = default;

QList<std::shared_ptr<Kalburator::Shape::ShapeContribution>>
MemoPlugin::shapeContributions() const
{
    // Timing contract: this runs at PluginManager load time (PalmRuntime ctor),
    // BEFORE createPalmBackend() populates m_categoryStore from the AppInfo block
    // at device-connect. The contribution/stages keep a borrowed POINTER (not a
    // snapshot), so by the time any transform runs (post-connect) the store is
    // populated. Do not cache the store by value, and do not assume it is
    // populated here.
    return { std::make_shared<NotePalmShapes>(m_categoryStore.get()) };
}

// --- Plugin identity ---

QString MemoPlugin::displayName() const { return QStringLiteral("Memos"); }
QIcon   MemoPlugin::icon()        const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-notes"));
}
QString MemoPlugin::description() const
{
    return QStringLiteral("Synchronizes Palm MemoDB with Markdown files");
}
QString MemoPlugin::version()     const { return QStringLiteral("2.0"); }

// --- Palm backend ---

std::unique_ptr<Kalburator::Sync::SyncBackendBase>
MemoPlugin::createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device)
{
    if (!device) return nullptr;
    m_palmBackend = std::make_unique<WildPalms::PalmSync::PalmBackend>(device);

    // F.3: populate the plugin-local category store for snapshot write-back
    // into Profile. MemoBlobBackend still receives nullptr — memo records
    // aren't routed by category, so this has no effect on sync behavior.
    WildPalms::PalmCalendar::populateFromAppInfo(
        *m_categoryStore,
        QStringLiteral("MemoDB"),
        m_palmBackend->readAppBlock(QStringLiteral("MemoDB")));

    return std::make_unique<MemoBlobBackend>(m_palmBackend.get(), /*categoryStore=*/nullptr);
}

QStringList MemoPlugin::categorySlotNames() const
{
    if (!m_categoryStore) return {};
    return m_categoryStore->sixteenSlotNames(primaryDbName());
}

WildPalms::PalmCalendar::CategoryMappingStore *
MemoPlugin::categoryStore() const
{
    return m_categoryStore.get();
}

// --- Main view ---

bool MemoPlugin::hasMainView() const { return true; }

QWidget *MemoPlugin::createMainView(QWidget *parent) const
{
    auto *v = new MemoView(parent);
    v->setHubReader(m_hubReader.get());
    if (m_runtime) {
        QObject::connect(m_runtime,
                         &WildPalms::Runtime::PalmRuntime::syncCompleted,
                         v, &MemoView::refresh);
    }
    return v;
}

void MemoPlugin::setHub(Kalburator::Sync::SyncBackendBase *hub)
{
    Q_ASSERT(hub);
    m_hubReader = std::make_unique<WildPalms::Memo::HubMemoReader>(
        hub, QStringLiteral("palm:memo"));
}

void MemoPlugin::setRuntime(WildPalms::Runtime::PalmRuntime *runtime)
{
    m_runtime = runtime;
}

QString MemoPlugin::mainViewName() const { return QStringLiteral("Memos"); }

QIcon MemoPlugin::mainViewIcon() const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-notes"));
}

// --- Conflict presentation ---

void MemoPlugin::enrichConflictSnapshot(
    Kalburator::Conflict::RecordSnapshot &snapshot,
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

QString MemoPlugin::formatConflictRecordHtml(
    const Kalburator::Conflict::RecordSnapshot &snapshot) const
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
