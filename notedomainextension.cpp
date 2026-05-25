#include "notedomainextension.h"

#include "palmnotetransformation.h"
#include "propertycatalogue.h"
#include "transformationregistry.h"

using namespace Kalburator::Shape;

namespace WildPalms::Memo {

namespace {

PropertyCatalogue makePalmCatalogue()
{
    PropertyCatalogue cat;
    // Palm MemoDB native fields; used by loss-profile UI to describe a palm-shape
    // record.
    cat.addProperty({ PropertyId{"body"},     PropertyKind::String,  QStringLiteral("Body") });
    cat.addProperty({ PropertyId{"private"},  PropertyKind::Boolean, QStringLiteral("Private") });
    cat.addProperty({ PropertyId{"category"}, PropertyKind::Integer, QStringLiteral("Category Slot") });
    return cat;
}

} // namespace

void NoteDomainExtension::registerWith(TransformationRegistry &registry)
{
    const Shape palm { DomainId{"note"}, EncodingId{"palm"} };
    const Shape canon{ DomainId{"note"}, EncodingId{"canon"} };

    // The registry is hub-and-spoke: a peer connects DIRECTLY to the canonical
    // hub, not to another peer. libkalburator's note domain declares (note,canon)
    // as the hub and owns the (note,markdown) spoke; we attach (note,palm) as a
    // second spoke with palm<->canon edges (each stage internally composes
    // memomarkdown with libkalburator's markdown<->canon). Routing to the on-disk
    // markdown peer is then palm->canon->markdown via the hub.
    //
    // Defensive: if libkalburator's canonical registration didn't run in this
    // address space (e.g. a unit test that wires the registry by hand), the canon
    // catalogue may be absent and registerEdge would assert "to-shape not
    // registered". Register a placeholder; registerShape is idempotent.
    if (registry.catalogueFor(canon) == nullptr) {
        registry.registerShape(canon, {});
    }
    registry.registerShape(palm, makePalmCatalogue());

    // palm -> canon (Reversible: identity rides in frontmatter -> providerExtras)
    registry.registerEdge(TransformationEdge{
        palm, canon, palmToCanonLoss(), std::make_shared<PalmToCanonStage>() });

    // canon -> palm (Simplified: Palm MemoDB holds plain text)
    registry.registerEdge(TransformationEdge{
        canon, palm, canonToPalmLoss(), std::make_shared<CanonToPalmStage>() });
}

} // namespace WildPalms::Memo
