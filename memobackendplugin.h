#ifndef WILDPALMS_MEMO_MEMOBACKENDPLUGIN_H
#define WILDPALMS_MEMO_MEMOBACKENDPLUGIN_H

#include <memory>

#include "plugin.h"

namespace Kalburator::Conflict { struct RecordSnapshot; }
namespace Kalburator::Sync { class SyncBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; }

class QIcon;
class QWidget;

namespace WildPalms::Memo {

/**
 * @brief Memo plugin (K.8b): inherits Kalburator::Plugin (K.7 surface).
 *
 * No longer a KCoreAddons MODULE plugin. Linked STATIC and loaded
 * in-process by PalmRuntime::registerPalmPlugins() (Task 6).
 *
 * Provides:
 *   - MemoBlobBackend (lifted to SyncBackend) via createPalmBackend()
 *     — called directly by PalmRuntime.
 *   - MemoView as a main-window tab.
 */
class MemoPlugin : public Kalburator::Plugin
{
public:
    MemoPlugin();
    ~MemoPlugin() override;

    // Kalburator::Plugin — Palm plugins don't contribute to libkalburator
    // BackendContribution system.
    QList<std::shared_ptr<Kalburator::Sync::BackendContribution>>
        backendContributions() const override { return {}; }

    // O7: contribute the (note, palm) peer shape + palm<->canon edges via the
    // shape-graph contribution system (PluginManager registers them into the
    // injected ShapeRegistries). Replaces the old ctor-time registerWith().
    QList<std::shared_ptr<Kalburator::Shape::ShapeContribution>>
        shapeContributions() const override;

    // Plugin identity
    QString     pluginId()         const { return QStringLiteral("memo"); }
    QString     displayName()      const;
    QIcon       icon()             const;
    QString     description()      const;
    QString     version()          const;
    QStringList claimedDatabases() const { return {QStringLiteral("MemoDB")}; }

    // F.3: Category slot snapshot — MemoPlugin keeps an internal
    // CategoryMappingStore purely for write-back into Profile.
    // MemoBlobBackend still receives nullptr (memos aren't routed by
    // category yet), so this has no effect on sync behavior.
    QString     primaryDbName()       const { return QStringLiteral("MemoDB"); }
    QStringList categorySlotNames()   const;

    // Palm backend — called directly by PalmRuntime (Task 6)
    std::unique_ptr<Kalburator::Sync::SyncBackend>
        createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device);

    // Main view
    bool     hasMainView()   const;
    QWidget *createMainView(QWidget *parent) const;
    QString  mainViewName()  const;
    QIcon    mainViewIcon()  const;

    // Conflict presentation (called by conflict UI layer)
    void    enrichConflictSnapshot(
        Kalburator::Conflict::RecordSnapshot &snapshot,
        bool isSourceSide) const;
    QString formatConflictRecordHtml(
        const Kalburator::Conflict::RecordSnapshot &snapshot) const;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmSync::PalmBackend> m_palmBackend;
};

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_MEMOBACKENDPLUGIN_H
