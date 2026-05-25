#include "memobackendplugin.h"

#include "memoblobbackend.h"
#include "memoview.h"
#include "notedomainextension.h"

#include "transformationregistry.h"

#include "palm/calendar/categoryappinforeader.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/codecs/memocodec.h"
#include "palm/sync/palmbackend.h"
#include "runtime/palmdeviceaccess.h"

#include "conflictrecord.h"   // Kalburator::Conflict::RecordSnapshot

#include <QIcon>
#include <QString>
#include <QWidget>

namespace WildPalms::Memo {

MemoPlugin::MemoPlugin()
    : m_categoryStore(
        std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
{
    // Phase 5: register the (note, palm) peer shape and palm<->markdown edges
    // with the process-wide TransformationRegistry at plugin construction
    // (mirrors CalendarBackendPlugin). Idempotent across instances.
    NoteDomainExtension::registerWith(
        Kalburator::Shape::TransformationRegistry::instance());
}
MemoPlugin::~MemoPlugin() = default;

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

std::unique_ptr<Kalburator::Sync::SyncBackend>
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

// --- Main view ---

bool MemoPlugin::hasMainView() const { return true; }

QWidget *MemoPlugin::createMainView(QWidget *parent) const
{
    return new MemoView(parent);
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
