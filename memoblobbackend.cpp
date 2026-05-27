#include "memoblobbackend.h"

#include "palm/sync/palmbackend.h"
#include "palm/sync/palmrecord.h"

#include "backendrecord.h"
#include "collectioninfo.h"

namespace WildPalms::Memo {

namespace {

// Phase 5: the backend presents raw (note, palm) wire bytes. The palm<->markdown
// conversion now lives in the shape-graph stages (NoteDomainExtension), not here.
Kalburator::Sync::BackendRecord palmToWireRecord(
    const WildPalms::PalmSync::PalmRecord &pr)
{
    Kalburator::Sync::BackendRecord br;
    br.id           = WildPalms::PalmSync::PalmBackend::encodeRecordId(
        QStringLiteral("MemoDB"), pr.recordId);
    br.type         = QStringLiteral("note");
    br.data         = pr.toWireBytes();
    br.contentHash  = pr.contentHash();
    br.lastModified = pr.lastModified;
    br.isDeleted    = pr.isDeleted();
    return br;
}

bool isMemoCollection(const QString &collectionId)
{
    // Accept the legacy per-db collection id ("palm:memo") and the domain-level
    // collection id ("palm:note") — both cover the entire MemoDB.
    return collectionId == QLatin1String(MemoBlobBackend::CollectionId)
        || collectionId == QLatin1String(MemoBlobBackend::DomainCollectionId);
}

} // namespace

MemoBlobBackend::MemoBlobBackend(
    WildPalms::PalmSync::PalmBackend *palmBackend,
    WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
    QObject *parent)
    : Kalburator::Sync::SyncBackend(parent)
    , m_palmBackend(palmBackend)
    , m_categoryStore(categoryStore)
{
}

MemoBlobBackend::~MemoBlobBackend() = default;

QString MemoBlobBackend::backendId()   const { return QStringLiteral("palm-memo"); }
QString MemoBlobBackend::displayName() const { return QStringLiteral("Palm Memos"); }
bool    MemoBlobBackend::isAvailable() const
{
    return m_palmBackend && m_palmBackend->isAvailable();
}

QList<Kalburator::Sync::CollectionInfo> MemoBlobBackend::availableCollections()
{
    // Domain-level collection: all MemoDB records, addressed by domain id.
    Kalburator::Sync::CollectionInfo domain;
    domain.id   = QLatin1String(DomainCollectionId);
    domain.name = QStringLiteral("Note");
    domain.type = QStringLiteral("note");

    // Legacy per-db collection id (kept for backwards compatibility).
    Kalburator::Sync::CollectionInfo legacy;
    legacy.id   = QLatin1String(CollectionId);
    legacy.name = QStringLiteral("Memos");
    legacy.type = QStringLiteral("memos");

    return { domain, legacy };
}

Kalburator::Sync::CollectionInfo MemoBlobBackend::collectionInfo(const QString &collectionId)
{
    if (!isMemoCollection(collectionId)) return {};
    // Return the entry whose id matches the request — both "palm:note" (domain)
    // and the legacy "palm:memo" alias resolve to the full MemoDB, but callers
    // that check the returned id must get back the id they asked for.
    for (const auto &c : availableCollections())
        if (c.id == collectionId) return c;
    return {};
}

QString MemoBlobBackend::createCollection(const Kalburator::Sync::CollectionInfo &)
{
    // Palm collections are device-allocated; not user-creatable through this API.
    return {};
}

QList<Kalburator::Sync::BackendRecord> MemoBlobBackend::loadRecords(
    const QString &collectionId)
{
    if (!m_palmBackend || !isMemoCollection(collectionId)) return {};

    QList<Kalburator::Sync::BackendRecord> out;
    for (const auto &pr : m_palmBackend->loadPalmRecords(QLatin1String(PalmDbName))) {
        out.append(palmToWireRecord(pr));
    }
    return out;
}

std::optional<Kalburator::Sync::BackendRecord> MemoBlobBackend::loadRecord(
    const QString &recordId)
{
    if (!m_palmBackend) return std::nullopt;
    QString dbName;
    std::uint32_t numericId = 0;
    if (!WildPalms::PalmSync::PalmBackend::decodeRecordId(
            recordId, &dbName, &numericId)) return std::nullopt;
    if (dbName != QLatin1String(PalmDbName)) return std::nullopt;

    const auto pr = m_palmBackend->loadPalmRecord(dbName, numericId);
    if (!pr.has_value()) return std::nullopt;
    return palmToWireRecord(*pr);
}

QString MemoBlobBackend::createRecord(const QString &collectionId,
                                      const Kalburator::Sync::BackendRecord &record)
{
    if (!m_palmBackend || !isMemoCollection(collectionId)) return {};
    if (record.data.isEmpty()) return {};

    auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(record.data);
    pr.recordId     = 0;   // device assigns
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();
    // pr.category is kept as decoded from the markdown frontmatter — memo carries
    // category per-record, not per-collection.

    const auto newId = m_palmBackend->createPalmRecord(QLatin1String(PalmDbName), pr);
    if (newId == 0) return {};
    return WildPalms::PalmSync::PalmBackend::encodeRecordId(
        QLatin1String(PalmDbName), newId);
}

bool MemoBlobBackend::updateRecord(const Kalburator::Sync::BackendRecord &record)
{
    if (!m_palmBackend) return false;
    QString dbName;
    std::uint32_t numericId = 0;
    if (!WildPalms::PalmSync::PalmBackend::decodeRecordId(
            record.id, &dbName, &numericId)) return false;
    if (dbName != QLatin1String(PalmDbName)) return false;
    if (record.data.isEmpty()) return false;

    auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(record.data);
    pr.recordId     = numericId;
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();

    return m_palmBackend->updatePalmRecord(dbName, pr);
}

bool MemoBlobBackend::deleteRecord(const QString &recordId)
{
    if (!m_palmBackend) return false;
    QString dbName;
    std::uint32_t numericId = 0;
    if (!WildPalms::PalmSync::PalmBackend::decodeRecordId(
            recordId, &dbName, &numericId)) {
        return false;
    }
    return m_palmBackend->deletePalmRecord(QStringLiteral("MemoDB"), numericId);
}

QList<Kalburator::Sync::BackendRecord> MemoBlobBackend::modifiedSince(
    const QString &collectionId, const QDateTime &since)
{
    if (!m_palmBackend || !isMemoCollection(collectionId)) return {};
    // Delegate to PalmBackend for the filter, then remap each to markdown.
    QList<Kalburator::Sync::BackendRecord> out;
    for (const auto &br : m_palmBackend->modifiedSince(collectionId, since)) {
        QString dbName;
        std::uint32_t numericId = 0;
        if (!WildPalms::PalmSync::PalmBackend::decodeRecordId(
                br.id, &dbName, &numericId)) continue;
        const auto pr = m_palmBackend->loadPalmRecord(dbName, numericId);
        if (!pr.has_value()) continue;
        out.append(palmToWireRecord(*pr));
    }
    return out;
}

QStringList MemoBlobBackend::deletedSince(const QString &collectionId, const QDateTime &since)
{
    if (!m_palmBackend || !isMemoCollection(collectionId)) return {};
    return m_palmBackend->deletedSince(collectionId, since);
}

bool MemoBlobBackend::supportsDeleteTracking() const
{
    return m_palmBackend ? m_palmBackend->supportsDeleteTracking() : false;
}

} // namespace WildPalms::Memo
