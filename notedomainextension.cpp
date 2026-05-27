#include "notedomainextension.h"

#include "palmnotetransformation.h"
#include <propertycatalogue.h>

using namespace Kalburator::Shape;

namespace WildPalms::Memo {

namespace {

PropertyCatalogue makePalmCatalogue()
{
    PropertyCatalogue cat;
    // Palm MemoDB native fields; used by loss-profile UI to describe a
    // palm-shape record.
    cat.addProperty({ PropertyId{"body"},     PropertyKind::String,  QStringLiteral("Body") });
    cat.addProperty({ PropertyId{"private"},  PropertyKind::Boolean, QStringLiteral("Private") });
    cat.addProperty({ PropertyId{"category"}, PropertyKind::Integer, QStringLiteral("Category Slot") });
    return cat;
}

} // namespace

DomainId NotePalmShapes::targetDomain() const
{
    return DomainId{QStringLiteral("note")};
}

QList<std::pair<Shape, PropertyCatalogue>> NotePalmShapes::peerShapes() const
{
    const Shape palm{ DomainId{"note"}, EncodingId{"palm"} };
    return { { palm, makePalmCatalogue() } };
}

QList<TransformationEdge> NotePalmShapes::edges() const
{
    const Shape palm { DomainId{"note"}, EncodingId{"palm"} };
    const Shape canon{ DomainId{"note"}, EncodingId{"canon"} };
    // palm -> canon (Reversible: identity rides in frontmatter -> providerExtras).
    // canon -> palm (Simplified: Palm MemoDB holds plain text).
    // The canon endpoint is registered by libkalburator's note domain plugin,
    // which loads earlier in the same PluginManager batch.
    return {
        TransformationEdge{ palm, canon, palmToCanonLoss(), std::make_shared<PalmToCanonStage>() },
        TransformationEdge{ canon, palm, canonToPalmLoss(), std::make_shared<CanonToPalmStage>() },
    };
}

} // namespace WildPalms::Memo
