#ifndef WILDPALMS_MEMO_MEMOBLOBBACKEND_H
#define WILDPALMS_MEMO_MEMOBLOBBACKEND_H

#include "syncbackend.h"

namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::Memo {

/**
 * @brief Transcoding SyncBackend wrapping PalmBackend's palm:memo.
 *
 * Exposes exactly one collection ("palm:memo") with type "memos".
 * loadRecords returns Markdown bytes (via MemoCodec -> MemoMarkdown);
 * createRecord / updateRecord accept Markdown bytes and encode back
 * to Palm wire format. Category slot is preserved via PalmBackend's
 * category-aware createPalmRecord / updatePalmRecord helpers.
 *
 * Non-owning pointers: palmBackend and categoryStore must outlive
 * the MemoBlobBackend. categoryStore may be null — when null the
 * encoder omits categoryName, and the decoder falls back to slot 0
 * for name-only `category:` strings.
 *
 * K.8b: lifted from IBlobBackend to SyncBackend directly (Task 3).
 * Calendar virtuals on SyncBackend are no-ops by default (K.4).
 */
class MemoBlobBackend : public Kalburator::Sync::SyncBackend
{
    Q_OBJECT
public:
    static constexpr const char *CollectionId = "palm:memo";
    static constexpr const char *PalmDbName   = "MemoDB";

    explicit MemoBlobBackend(
        WildPalms::PalmSync::PalmBackend *palmBackend,
        WildPalms::PalmCalendar::CategoryMappingStore *categoryStore = nullptr,
        QObject *parent = nullptr);
    ~MemoBlobBackend() override;

    // --- SyncBackendBase identity (pure virtuals) ---
    QString backendType() const override { return QStringLiteral("palm-memo"); }
    // K.8b T6 fix: use blob/raw to match the pre-T3 BlobBackendAdapter
    // wrapping that tests expect.  loadRecords() already returns Markdown
    // bytes (transcoded by MemoCodec), so the engine copies them verbatim
    // via the identity (blob,raw) -> (blob,raw) pipeline.  A dedicated
    // memo/text shape requires a registered transcoder that does not yet
    // exist; revert to blob/raw until Task 7 deletes BlobBackendAdapter
    // and the test infrastructure is updated.
    QList<Kalburator::Shape::Shape> nativeShapes() const override {
        return { { Kalburator::Shape::DomainId{QStringLiteral("blob")},
                   Kalburator::Shape::EncodingId{QStringLiteral("raw")} } };
    }

    // --- IBlobBackend identity (override SyncBackendBase defaults) ---
    QString backendId()   const override;
    QString displayName() const override;
    bool    isAvailable() const override;

    // --- Collections ---
    QList<Kalburator::Sync::CollectionInfo> availableCollections() override;
    Kalburator::Sync::CollectionInfo collectionInfo(const QString &collectionId) override;
    QString createCollection(const Kalburator::Sync::CollectionInfo &info) override;

    // --- Records ---
    QList<Kalburator::Sync::BackendRecord> loadRecords(const QString &collectionId) override;
    std::optional<Kalburator::Sync::BackendRecord> loadRecord(const QString &recordId) override;
    QString createRecord(const QString &collectionId,
                         const Kalburator::Sync::BackendRecord &record) override;
    bool    updateRecord(const Kalburator::Sync::BackendRecord &record) override;
    bool    deleteRecord(const QString &recordId) override;

    // --- Change detection ---
    QList<Kalburator::Sync::BackendRecord> modifiedSince(
        const QString &collectionId, const QDateTime &since) override;
    QStringList deletedSince(const QString &collectionId, const QDateTime &since) override;
    bool        supportsDeleteTracking() const override;

Q_SIGNALS:
    void recordCreated(const QString &recordId);
    void recordUpdated(const QString &recordId);
    void recordDeleted(const QString &recordId);
    void errorOccurred(const QString &error);
    void progressUpdated(int current, int total, const QString &message);

private:
    WildPalms::PalmSync::PalmBackend              *m_palmBackend = nullptr;
    WildPalms::PalmCalendar::CategoryMappingStore *m_categoryStore = nullptr;
};

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_MEMOBLOBBACKEND_H
