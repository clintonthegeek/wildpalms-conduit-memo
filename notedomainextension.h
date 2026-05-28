#ifndef WILDPALMS_MEMO_NOTEDOMAINEXTENSION_H
#define WILDPALMS_MEMO_NOTEDOMAINEXTENSION_H

#include <shapecontribution.h>

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::Memo {

// O7: contributes the (note, palm) peer shape plus palm<->canon edges to the
// shape graph. libkalburator owns (note,canon) and the (note,markdown) spoke;
// each palm<->canon stage internally composes memomarkdown with libkalburator's
// markdown<->canon. PluginManager registers this into the injected ShapeRegistries.
//
// Borrows a (nullable) CategoryMappingStore, threaded into the palm<->canon
// stages so the Palm category slot rides as the category NAME in the YAML
// frontmatter. The store is owned by the plugin and outlives this contribution.
class NotePalmShapes : public Kalburator::Shape::ShapeContribution {
public:
    explicit NotePalmShapes(
        const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);

    Kalburator::Shape::DomainId targetDomain() const override;
    QList<std::pair<Kalburator::Shape::Shape, Kalburator::Shape::PropertyCatalogue>>
        peerShapes() const override;
    QList<Kalburator::Shape::TransformationEdge> edges() const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_NOTEDOMAINEXTENSION_H
