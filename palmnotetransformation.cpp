#include "palmnotetransformation.h"

#include "memomarkdown.h"
#include "markdowncanonstages.h"   // libkalburator Kalburator::Note markdown<->canon
#include "palm/codecs/memocodec.h"
#include "palm/sync/palmrecord.h"
#include "palm/calendar/categorymappingstore.h"

using namespace Kalburator::Shape;

namespace WildPalms::Memo {

// "MemoDB" is the single Palm database name for the Memo application; hardcoded
// here because each database has its own category AppInfo block and this plugin
// exclusively handles MemoDB records.
static const QString kMemoDbName = QStringLiteral("MemoDB");

PalmToCanonStage::PalmToCanonStage(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray PalmToCanonStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    const auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(sourceBytes);

    MarkdownMemo m;
    m.recordId     = pr.recordId;
    m.categorySlot = pr.category;
    m.content      = WildPalms::PalmCodecs::decodeMemo(pr.data)
                         .value_or(WildPalms::PalmCodecs::Memo{});
    m.content.isPrivate =
        (pr.attributes & WildPalms::PalmSync::PalmRecord::AttrSecret) != 0;

    // palm -> markdown (memomarkdown), then markdown -> canon (libkalburator). The
    // entire frontmatter block rides verbatim into providerExtras["frontmatter"].
    // When m_cats is non-null, encode() writes the category NAME (not slot index)
    // so cross-domain routing can match by name.
    const QByteArray markdown = encode(m, m_cats, kMemoDbName).toUtf8();
    return Kalburator::Note::MarkdownToCanonStage{}.transform(markdown);
}

CanonToPalmStage::CanonToPalmStage(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray CanonToPalmStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};

    // canon -> markdown (libkalburator, re-emits frontmatter verbatim), then
    // markdown -> palm (memomarkdown). When m_cats is non-null, decode() resolves
    // the category NAME in the frontmatter back to a Palm slot index.
    const QByteArray markdown =
        Kalburator::Note::CanonToMarkdownStage{}.transform(sourceBytes);
    const MarkdownMemo m = decode(QString::fromUtf8(markdown), m_cats, kMemoDbName);

    WildPalms::PalmSync::PalmRecord pr;
    pr.recordId = m.recordId;   // from frontmatter id:; 0 if absent
    pr.category = static_cast<std::uint8_t>(m.categorySlot);
    pr.data     = WildPalms::PalmCodecs::encodeMemo(m.content);
    if (m.content.isPrivate)
        pr.attributes |= WildPalms::PalmSync::PalmRecord::AttrSecret;
    return pr.toWireBytes();
}

LossProfile palmToCanonLoss()
{
    // palm -> markdown is lossless; markdown -> canon carries the frontmatter
    // (recordId / category slot / private flag) verbatim into providerExtras —
    // a round-trippable side channel, so Reversible. Nothing is dropped.
    LossProfile p;
    p.affected.insert(PropertyId{QStringLiteral("frontmatter")}, LossKind::Reversible);
    return p;
}

LossProfile canonToPalmLoss()
{
    LossProfile p;
    // canon -> markdown re-emits the frontmatter verbatim (Reversible side channel)
    p.affected.insert(PropertyId{QStringLiteral("frontmatter")}, LossKind::Reversible);
    // ...markdown -> palm flattens any Markdown structure to Palm plain text.
    p.affected.insert(PropertyId{QStringLiteral("body")}, LossKind::Simplified);
    // The canon multi-value `categories` list has no Palm equivalent (Palm holds a
    // single category slot, which rides in providerExtras frontmatter — not the
    // canon `categories` property).
    p.affected.insert(PropertyId{QStringLiteral("categories")}, LossKind::Dropped);
    return p;
}

} // namespace WildPalms::Memo
