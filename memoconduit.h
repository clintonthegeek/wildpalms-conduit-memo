#ifndef MEMOCONDUIT_H
#define MEMOCONDUIT_H

#include "sync/conduit.h"

namespace Sync {

/**
 * @brief Conduit for Palm Memos ↔ Markdown files
 *
 * Syncs:
 *   - Palm MemoDB (binary format)
 *   - Local .md files with YAML frontmatter
 *
 * Uses MemoMapper for format conversion.
 * Supports bidirectional category sync.
 */
class MemoConduit : public SyncConduitBase
{
    Q_OBJECT

public:
    explicit MemoConduit(QObject *parent = nullptr);

    // ========== Conduit Identity ==========

    QString conduitId() const override { return "memos"; }
    QString displayName() const override { return "Memos"; }
    QStringList palmDatabaseNames() const override { return {"MemoDB"}; }
    QString fileExtension() const override { return ".md"; }

    // ========== UI Contribution ==========
    QIcon icon() const override {
        return QIcon::fromTheme(QStringLiteral("view-pim-notes"));
    }
    QString description() const override {
        return QStringLiteral("Synchronizes Palm MemoDB with Markdown files");
    }
    bool hasView() const override { return true; }
    QWidget *createView(QWidget *parent) override;
    QString viewName() const override { return QStringLiteral("Memos"); }
    QIcon viewIcon() const override {
        return QIcon::fromTheme(QStringLiteral("view-pim-notes"));
    }

    // ========== Record Conversion ==========

    BackendRecord* palmToBackend(PilotRecord *palmRecord,
                                  SyncContext *context) override;

    PilotRecord* backendToPalm(BackendRecord *backendRecord,
                                SyncContext *context) override;

    bool recordsEqual(PilotRecord *palm, BackendRecord *backend,
                       const SyncContext *context) const override;

    QString palmRecordDescription(PilotRecord *record,
                                   const SyncContext *context) const override;

    // ========== Conflict Display ==========

    void enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                 bool isSourceSide) const override;
    QString formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const override;
};

} // namespace Sync

#endif // MEMOCONDUIT_H
