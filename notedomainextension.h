#ifndef WILDPALMS_MEMO_NOTEDOMAINEXTENSION_H
#define WILDPALMS_MEMO_NOTEDOMAINEXTENSION_H

namespace Kalburator::Shape { class TransformationRegistry; }

namespace WildPalms::Memo {

// Registers the (note, palm) peer shape plus palm<->markdown edges with a
// TransformationRegistry. Idempotent (registerShape/registerEdge tolerate
// re-registration). libkalburator owns (note,markdown)<->(note,canon); we own
// palm<->markdown.
class NoteDomainExtension {
public:
    static void registerWith(Kalburator::Shape::TransformationRegistry &registry);
};

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_NOTEDOMAINEXTENSION_H
