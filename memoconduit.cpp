#include "memoconduit.h"
#include "memoview.h"
#include "memomapper.h"
#include "palm/pilotrecord.h"
#include "palm/categoryinfo.h"
#include "sync/localfilebackend.h"
#include "sync/qsynccore/conflictrecord.h"

#include <QDebug>

namespace Sync {

MemoConduit::MemoConduit(QObject *parent)
    : SyncConduitBase(parent)
{
}

BackendRecord* MemoConduit::palmToBackend(PilotRecord *palmRecord,
                                           SyncContext *context)
{
    if (!palmRecord) return nullptr;

    // Unpack Palm memo
    MemoMapper::Memo memo = MemoMapper::unpackMemo(palmRecord);

    // Convert to Markdown
    QString catName = categoryName(memo.category);
    qDebug() << "[MemoConduit::palmToBackend] Palm ID:" << palmRecord->id()
             << "category index:" << memo.category
             << "-> name:" << catName
             << "m_categories valid:" << (m_categories != nullptr)
             << "m_categories->isValid():" << (m_categories ? m_categories->isValid() : false);
    QString markdown = MemoMapper::memoToMarkdown(memo, catName);

    // Create backend record
    BackendRecord *record = new BackendRecord();
    record->data = markdown.toUtf8();
    record->type = "memo";
    record->contentHash = LocalFileBackend::calculateHash(record->data);
    record->lastModified = QDateTime::currentDateTime();

    // Set display name from first line of memo
    QString text = memo.text.trimmed();
    int newlinePos = text.indexOf('\n');
    if (newlinePos > 0) {
        text = text.left(newlinePos).trimmed();
    }
    if (text.length() > 50) {
        text = text.left(50);
    }
    record->displayName = text;

    return record;
}

PilotRecord* MemoConduit::backendToPalm(BackendRecord *backendRecord,
                                         SyncContext *context)
{
    if (!backendRecord) return nullptr;

    // Parse Markdown content
    QString content = QString::fromUtf8(backendRecord->data);
    MemoMapper::Memo memo = MemoMapper::markdownToMemo(content);

    // Look up or create category from name
    if (!memo.categoryName.isEmpty() && m_categories) {
        memo.category = m_categories->getOrCreateCategory(memo.categoryName);
        qDebug() << "[MemoConduit] Category" << memo.categoryName << "-> index" << memo.category;
    }

    // Pack to Palm record
    PilotRecord *record = MemoMapper::packMemo(memo);

    return record;
}

bool MemoConduit::recordsEqual(PilotRecord *palm, BackendRecord *backend) const
{
    if (!palm || !backend) return false;

    // Unpack Palm memo
    MemoMapper::Memo palmMemo = MemoMapper::unpackMemo(palm);

    // Parse backend content
    QString backendContent = QString::fromUtf8(backend->data);
    MemoMapper::Memo backendMemo = MemoMapper::markdownToMemo(backendContent);

    // Compare text content (main comparison)
    QString palmText = palmMemo.text.trimmed();
    QString backendText = backendMemo.text.trimmed();

    if (palmText != backendText) {
        return false;
    }

    // Compare categories
    QString palmCategoryName = categoryName(palmMemo.category);

    // Normalize: "Unfiled" (index 0) and empty string are equivalent
    QString normalizedPalmCat = palmCategoryName;
    QString normalizedBackendCat = backendMemo.categoryName;

    if (normalizedPalmCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedPalmCat.clear();
    }
    if (normalizedBackendCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedBackendCat.clear();
    }

    if (normalizedPalmCat.compare(normalizedBackendCat, Qt::CaseInsensitive) != 0) {
        return false;
    }

    return true;
}

QString MemoConduit::palmRecordDescription(PilotRecord *record) const
{
    if (!record) return QString();

    MemoMapper::Memo memo = MemoMapper::unpackMemo(record);

    // Use first line as description
    QString text = memo.text.trimmed();
    int newlinePos = text.indexOf('\n');
    if (newlinePos > 0) {
        text = text.left(newlinePos).trimmed();
    }

    // Limit length
    if (text.length() > 60) {
        text = text.left(57) + "...";
    }

    return text;
}

void MemoConduit::enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                          bool isSourceSide) const
{
    if (snapshot.content.isEmpty()) return;

    if (isSourceSide) {
        // Source: Palm binary — unpack via mapper
        PilotRecord tempRecord(0, 0, 0, snapshot.content);
        MemoMapper::Memo memo = MemoMapper::unpackMemo(&tempRecord);
        snapshot.content = memo.text.toUtf8();
    }

    // Extract title from first line
    QString text = QString::fromUtf8(snapshot.content).trimmed();
    int newlinePos = text.indexOf('\n');
    QString title = (newlinePos > 0) ? text.left(newlinePos).trimmed() : text;
    if (title.length() > 60)
        title = title.left(57) + QStringLiteral("...");

    snapshot.metadata[QStringLiteral("title")] = title;
    snapshot.contentType = QStringLiteral("text/plain");
}

QString MemoConduit::formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const
{
    QString html;
    QString title = snapshot.metadata.value(QStringLiteral("title")).toString();
    if (!title.isEmpty()) {
        html += QStringLiteral("<h3>%1</h3>").arg(title.toHtmlEscaped());
    }
    QString content = QString::fromUtf8(snapshot.content);
    html += QStringLiteral("<pre>%1</pre>").arg(content.toHtmlEscaped());
    return html;
}

QWidget *MemoConduit::createView(QWidget *parent)
{
    return new MemoView(parent);
}

} // namespace Sync

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(MemoConduitFactory, "memo-conduit.json",
                           registerPlugin<Sync::MemoConduit>();)

#include "memoconduit.moc"
