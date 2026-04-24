#include "memoblobbackend.h"

#include <QCryptographicHash>

#include "memomarkdown.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/codecs/memocodec.h"
#include "palm/sync/palmbackend.h"
#include "palm/sync/palmrecord.h"

#include "backendrecord.h"
#include "collectioninfo.h"

namespace WildPalms::Memo {

namespace {

QString sha256Hex(const QByteArray &bytes)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

Kalburator::Sync::BackendRecord palmToMarkdownRecord(
    const WildPalms::PalmSync::PalmRecord &pr,
    const WildPalms::PalmCalendar::CategoryMappingStore *store)
{
    const auto decoded = WildPalms::PalmCodecs::decodeMemo(pr.data);
    WildPalms::Memo::MarkdownMemo m;
    m.recordId = pr.recordId;
    m.categorySlot = pr.category;
    m.content = decoded.value_or(WildPalms::PalmCodecs::Memo{});
    m.content.isPrivate = (pr.attributes & WildPalms::PalmSync::PalmRecord::AttrSecret) != 0;
    if (store) {
        const QString name = store->slotName(QStringLiteral("MemoDB"), pr.category);
        if (!name.isEmpty() && pr.category != 0) {
            m.categoryName = name;
        }
    }

    const QByteArray bytes = WildPalms::Memo::encode(m).toUtf8();
    Kalburator::Sync::BackendRecord br;
    br.id   = WildPalms::PalmSync::PalmBackend::encodeRecordId(
        QStringLiteral("MemoDB"), pr.recordId);
    br.type = QStringLiteral("memos");
    br.data = bytes;
    br.contentHash = sha256Hex(bytes);
    br.lastModified = pr.lastModified;
    br.isDeleted = pr.isDeleted();
    return br;
}

WildPalms::PalmSync::PalmRecord markdownRecordToPalm(
    const Kalburator::Sync::BackendRecord &br,
    std::uint32_t existingId,
    const WildPalms::PalmCalendar::CategoryMappingStore *store)
{
    WildPalms::Memo::MarkdownMemo m = WildPalms::Memo::decode(QString::fromUtf8(br.data));
    if (m.recordId == 0) m.recordId = existingId;

    // If frontmatter gave us a name but not a slot, try to resolve slot
    // from the store (keyed on MemoDB).
    if (m.categorySlot == 0 && m.categoryName.has_value() && store) {
        // CategoryMappingStore doesn't offer a reverse lookup; walk 1..15.
        for (int slot = 1; slot < 16; ++slot) {
            const QString name = store->slotName(QStringLiteral("MemoDB"), slot);
            if (name.compare(*m.categoryName, Qt::CaseInsensitive) == 0) {
                m.categorySlot = slot;
                break;
            }
        }
    }

    WildPalms::PalmSync::PalmRecord pr;
    pr.recordId = m.recordId;   // 0 on create, non-zero on update
    pr.category = static_cast<std::uint8_t>(m.categorySlot);
    pr.data     = WildPalms::PalmCodecs::encodeMemo(m.content);
    if (m.content.isPrivate) {
        pr.attributes |= WildPalms::PalmSync::PalmRecord::AttrSecret;
    }
    pr.lastModified = br.lastModified.isValid()
        ? br.lastModified
        : QDateTime::currentDateTimeUtc();
    return pr;
}

bool isMemoCollection(const QString &collectionId)
{
    return collectionId == QLatin1String(MemoBlobBackend::CollectionId);
}

} // namespace

MemoBlobBackend::MemoBlobBackend(
    WildPalms::PalmSync::PalmBackend *palmBackend,
    WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
    QObject *parent)
    : Kalburator::Sync::IBlobBackend(parent)
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
    Kalburator::Sync::CollectionInfo info;
    info.id   = QLatin1String(CollectionId);
    info.name = QStringLiteral("Memos");
    info.type = QStringLiteral("memos");
    return { info };
}

Kalburator::Sync::CollectionInfo MemoBlobBackend::collectionInfo(const QString &collectionId)
{
    if (!isMemoCollection(collectionId)) return {};
    return availableCollections().first();
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
        out.append(palmToMarkdownRecord(pr, m_categoryStore));
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
    return palmToMarkdownRecord(*pr, m_categoryStore);
}

QString MemoBlobBackend::createRecord(const QString &collectionId,
                                      const Kalburator::Sync::BackendRecord &record)
{
    if (!m_palmBackend || !isMemoCollection(collectionId)) return {};
    const auto pr = markdownRecordToPalm(record, /*existingId=*/0, m_categoryStore);
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

    const auto pr = markdownRecordToPalm(record, numericId, m_categoryStore);
    return m_palmBackend->updatePalmRecord(dbName, pr);
}

bool MemoBlobBackend::deleteRecord(const QString &recordId)
{
    if (!m_palmBackend) return false;
    return m_palmBackend->deleteRecord(recordId);
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
        out.append(palmToMarkdownRecord(*pr, m_categoryStore));
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
