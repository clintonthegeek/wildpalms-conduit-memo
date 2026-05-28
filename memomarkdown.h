#ifndef WILDPALMS_MEMO_MEMOMARKDOWN_H
#define WILDPALMS_MEMO_MEMOMARKDOWN_H

#include <cstdint>
#include <optional>

#include <QString>

#include "palm/codecs/memocodec.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

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
///
/// Without a store (cats == nullptr): writes `category: <slot>` (numeric) when
/// slot is non-zero, plus `categoryName: <name>` when categoryName is set.
///
/// With a store: when slot != 0, resolves the slot to a name via
/// cats->slotName(dbName, slot) and writes `category: <name>` (the human-
/// readable string). This is the name-based model used by the sync path so
/// that cross-domain category routing works by name, not by volatile slot
/// numbers. In this mode `categoryName` is ignored (the store is canonical).
/// Falls back to numeric if the store returns an empty name for the slot.
QString encode(const MarkdownMemo &memo,
               const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr,
               const QString &dbName = {});

/// Parse a Markdown file. Tolerates missing frontmatter, malformed
/// frontmatter lines, integer or string `category:` values, and
/// unknown keys (including the legacy `created:` field the old
/// memomapper emitted).
///
/// Without a store: a string `category:` value is kept in categoryName;
/// categorySlot stays 0 (caller must resolve).
///
/// With a store: a string `category:` value is resolved to a slot via
/// cats->slotForName(dbName, name) and stored in categorySlot directly.
MarkdownMemo decode(const QString &markdown,
                    const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr,
                    const QString &dbName = {});

/// Human-friendly filename derived from the first line of the memo
/// body. Falls back to "memo_<recordId>.md" for empty bodies or
/// bodies that sanitise to nothing useful.
QString filenameFor(const MarkdownMemo &memo);

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_MEMOMARKDOWN_H
