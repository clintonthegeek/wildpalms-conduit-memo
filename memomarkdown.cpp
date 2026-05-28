#include "memomarkdown.h"

#include <QRegularExpression>
#include <QStringList>

#include "palm/calendar/categorymappingstore.h"

namespace WildPalms::Memo {

namespace {

QString sanitiseFilenameStem(const QString &input)
{
    static const QRegularExpression invalidChars("[^a-zA-Z0-9_\\-. ]");
    static const QRegularExpression multiSpace("\\s+");

    QString stem = input;
    stem.replace(invalidChars, QStringLiteral("_"));
    stem.replace(multiSpace, QStringLiteral("_"));
    while (stem.startsWith('_')) stem.remove(0, 1);
    while (stem.endsWith('_'))   stem.chop(1);
    return stem.trimmed();
}

} // namespace

QString encode(const MarkdownMemo &memo,
               const WildPalms::PalmCalendar::CategoryMappingStore *cats,
               const QString &dbName)
{
    QString out;
    out.reserve(128 + memo.content.text.size());

    out += QStringLiteral("---\n");
    out += QStringLiteral("id: %1\n").arg(memo.recordId);

    if (cats && memo.categorySlot != 0) {
        // Name-based model: write the human-readable category name so that
        // cross-domain routing can match by name, not by volatile slot index.
        const QString name = cats->slotName(dbName, memo.categorySlot);
        if (!name.isEmpty()) {
            out += QStringLiteral("category: %1\n").arg(name);
        } else {
            // Store doesn't know this slot yet — fall back to numeric.
            out += QStringLiteral("category: %1\n").arg(memo.categorySlot);
        }
    } else {
        // No store: legacy / store-free path. Write numeric slot + optional name.
        const bool hasCategoryName =
            memo.categoryName.has_value() && !memo.categoryName->isEmpty();
        if (memo.categorySlot != 0 || hasCategoryName) {
            out += QStringLiteral("category: %1\n").arg(memo.categorySlot);
        }
        if (hasCategoryName) {
            out += QStringLiteral("categoryName: %1\n").arg(*memo.categoryName);
        }
    }

    if (memo.content.isPrivate) {
        out += QStringLiteral("private: true\n");
    }

    out += QStringLiteral("---\n\n");
    out += memo.content.text;
    if (!out.endsWith(QChar('\n'))) {
        out += QChar('\n');
    }
    return out;
}

MarkdownMemo decode(const QString &markdown,
                    const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                    const QString &dbName)
{
    MarkdownMemo m;

    QString body;
    if (markdown.startsWith(QStringLiteral("---"))) {
        const int end = markdown.indexOf(QStringLiteral("\n---"), 3);
        if (end != -1) {
            const QString frontmatter = markdown.mid(4, end - 4);
            body = markdown.mid(end + 5);
            while (body.startsWith(QChar('\n'))) body.remove(0, 1);

            const QStringList lines = frontmatter.split(QChar('\n'));
            for (const QString &line : lines) {
                const int colon = line.indexOf(QChar(':'));
                if (colon == -1) continue;

                const QString key   = line.left(colon).trimmed().toLower();
                const QString value = line.mid(colon + 1).trimmed();

                if (key == QStringLiteral("id")) {
                    m.recordId = static_cast<std::uint32_t>(value.toUInt());
                } else if (key == QStringLiteral("category")) {
                    bool ok = false;
                    const int asInt = value.toInt(&ok);
                    if (ok) {
                        m.categorySlot = asInt;
                    } else if (cats) {
                        // Name-based model: resolve name -> slot via the store.
                        m.categorySlot = cats->slotForName(dbName, value);
                        m.categoryName = value;
                    } else {
                        // No store: park the name; slot stays 0.
                        m.categoryName = value;
                    }
                } else if (key == QStringLiteral("categoryname")) {
                    m.categoryName = value;
                } else if (key == QStringLiteral("private")) {
                    m.content.isPrivate = (value.toLower() == QStringLiteral("true"));
                }
                // Unknown keys (created:, modified:, etc.) silently ignored.
            }
        } else {
            body = markdown;
        }
    } else {
        body = markdown;
    }

    if (body.endsWith(QChar('\n'))) body.chop(1);
    m.content.text = body;
    return m;
}

QString filenameFor(const MarkdownMemo &memo)
{
    const QString firstLine = memo.content.text.split(QChar('\n')).first().trimmed();

    QString stem;
    if (firstLine.isEmpty()) {
        stem = QStringLiteral("memo_%1").arg(memo.recordId);
    } else {
        stem = sanitiseFilenameStem(firstLine.left(50));
        if (stem.isEmpty()) {
            stem = QStringLiteral("memo_%1").arg(memo.recordId);
        }
    }
    return stem + QStringLiteral(".md");
}

} // namespace WildPalms::Memo
