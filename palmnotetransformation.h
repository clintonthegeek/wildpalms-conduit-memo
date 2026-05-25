#ifndef WILDPALMS_MEMO_PALMNOTETRANSFORMATION_H
#define WILDPALMS_MEMO_PALMNOTETRANSFORMATION_H

#include "transformationedge.h"

namespace WildPalms::Memo {

// (note, palm) -> (note, canon): Palm MemoDB wire bytes to the note canon. Wraps
// memomarkdown (palm->markdown) composed with libkalburator's MarkdownToCanonStage
// (markdown->canon). The registry is hub-and-spoke: a peer must connect directly
// to the canonical, so this single edge spans palm->markdown->canon. Identity
// (recordId/category slot/private) rides in the YAML frontmatter, carried verbatim
// into the canon's providerExtras["frontmatter"] (Reversible).
class PalmToCanonStage : public Kalburator::Shape::TransformationStage {
public:
    QByteArray transform(const QByteArray &sourceBytes) const override;
};

// (note, canon) -> (note, palm): canon to Palm MemoDB wire bytes. Wraps
// libkalburator's CanonToMarkdownStage (canon->markdown) composed with memomarkdown
// (markdown->palm). Markdown structure is flattened to Palm plain text.
class CanonToPalmStage : public Kalburator::Shape::TransformationStage {
public:
    QByteArray transform(const QByteArray &sourceBytes) const override;
};

Kalburator::Shape::LossProfile palmToCanonLoss();
Kalburator::Shape::LossProfile canonToPalmLoss();

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_PALMNOTETRANSFORMATION_H
