#ifndef WILDPALMS_MEMO_HUBMEMOREADER_H
#define WILDPALMS_MEMO_HUBMEMOREADER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Kalburator::Sync { class SyncBackendBase; }

namespace WildPalms::Memo {

/**
 * @brief Thin per-domain facade over the canonical hub
 *        ("wp-hub" GenericSqliteBackend, collection "palm:memo").
 *
 * Read-only. The view (MemoView) talks to this facade -- never
 * to Kalburator::Sync::SyncBackendBase directly. The reader owns no Qt
 * signals; refresh is driven by PalmRuntime::syncCompleted.
 *
 * Lifetime: the hub pointer is borrowed and outlives the reader.
 * The reader is owned by MemoPlugin; the plugin outlives the view
 * that borrows the reader pointer.
 */
class HubMemoReader {
public:
    HubMemoReader(Kalburator::Sync::SyncBackendBase *hub,
                  QString collectionId);

    QStringList listRecordIds() const;
    QByteArray  recordBytes(const QString &id) const;
    QString     collectionId() const { return m_collectionId; }

private:
    Kalburator::Sync::SyncBackendBase *m_hub;
    QString m_collectionId;
};

} // namespace WildPalms::Memo

#endif
