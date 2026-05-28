#ifndef WILDPALMS_MEMO_PALMNOTETRANSFORMATION_H
#define WILDPALMS_MEMO_PALMNOTETRANSFORMATION_H

#include "transformationedge.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::Memo {

// (note, palm) -> (note, canon): Palm MemoDB wire bytes to the note canon. Wraps
// memomarkdown (palm->markdown) composed with libkalburator's MarkdownToCanonStage
// (markdown->canon). The registry is hub-and-spoke: a peer must connect directly
// to the canonical, so this single edge spans palm->markdown->canon. Identity
// (recordId/category slot/private) rides in the YAML frontmatter, carried verbatim
// into the canon's providerExtras["frontmatter"] (Reversible).
//
// Borrows a (nullable) CategoryMappingStore to write the Palm category slot as a
// human-readable name in the YAML frontmatter. The store must outlive this stage.
// When cats == nullptr the stage degrades gracefully: slot 0 on decode, no
// category line in the frontmatter for the name-based model.
class PalmToCanonStage : public Kalburator::Shape::TransformationStage {
public:
    explicit PalmToCanonStage(
        const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

// (note, canon) -> (note, palm): canon to Palm MemoDB wire bytes. Wraps
// libkalburator's CanonToMarkdownStage (canon->markdown) composed with memomarkdown
// (markdown->palm). Markdown structure is flattened to Palm plain text.
//
// Borrows the same (nullable) CategoryMappingStore to resolve the category name in
// the frontmatter back to a Palm slot index. The store must outlive this stage.
class CanonToPalmStage : public Kalburator::Shape::TransformationStage {
public:
    explicit CanonToPalmStage(
        const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

Kalburator::Shape::LossProfile palmToCanonLoss();
Kalburator::Shape::LossProfile canonToPalmLoss();

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_PALMNOTETRANSFORMATION_H
