#ifndef WILDPALMS_MEMO_MEMOMARKDOWN_H
#define WILDPALMS_MEMO_MEMOMARKDOWN_H

#include <cstdint>
#include <optional>

#include <QString>

#include "palm/codecs/memocodec.h"

namespace WildPalms::Memo {

/// Bundle of "everything a Markdown file on disk represents about a
/// memo." Content (text + isPrivate) is the Palm-facing POD from
/// MemoCodec. Everything else is file-layer state: record id for
/// round-trip, category slot (authoritative), optional
/// human-readable category name (decorative).
struct MarkdownMemo {
    WildPalms::PalmCodecs::Memo content;
    std::uint32_t               recordId = 0;
    int                         categorySlot = 0;     // 0..15
    std::optional<QString>      categoryName;          // empty => omit
};

/// Encode to Markdown with canonical YAML frontmatter. Produces
/// hash-stable bytes: same MarkdownMemo always yields the same
/// QString. Omits default-valued fields (slot 0, private=false,
/// missing categoryName). Body ends with exactly one \n.
QString encode(const MarkdownMemo &memo);

/// Parse a Markdown file. Tolerates missing frontmatter, malformed
/// frontmatter lines, integer or string `category:` values, and
/// unknown keys (including the legacy `created:` field the old
/// memomapper emitted).
MarkdownMemo decode(const QString &markdown);

/// Human-friendly filename derived from the first line of the memo
/// body. Falls back to "memo_<recordId>.md" for empty bodies or
/// bodies that sanitise to nothing useful.
QString filenameFor(const MarkdownMemo &memo);

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_MEMOMARKDOWN_H
