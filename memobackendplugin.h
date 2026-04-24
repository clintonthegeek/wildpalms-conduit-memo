#ifndef WILDPALMS_MEMO_MEMOBACKENDPLUGIN_H
#define WILDPALMS_MEMO_MEMOBACKENDPLUGIN_H

#include "core/ibackendplugin.h"

#include <QObject>

namespace WildPalms::Memo {

/**
 * @brief First new-ABI WildPalms plugin.
 *
 * Provides a MemoBlobBackend wrapping the shared PalmBackend the
 * runtime owns (reached via PalmDeviceConnection::palmBackend).
 * Surfaces MemoView as a main-window tab.
 */
class MemoBackendPlugin : public QObject, public WildPalms::IBackendPlugin
{
    Q_OBJECT
    Q_INTERFACES(WildPalms::IBackendPlugin)
public:
    explicit MemoBackendPlugin(QObject *parent = nullptr);
    ~MemoBackendPlugin() override;

    // IPlugin
    QString pluginId()    const override;
    QString displayName() const override;
    QIcon   icon()        const override;
    QString description() const override;
    QString version()     const override;

    // IBackendPlugin
    QStringList      claimedDatabases() const override;
    ProvidedBackends createBackends(Kalburator::Sync::ISyncHost *host,
                                    PalmDeviceConnection         *device) override;

    // IBackendPlugin — main view
    bool     hasMainView()   const override;
    QWidget *createMainView(QWidget *parent) const override;
    QString  mainViewName()  const override;
    QIcon    mainViewIcon()  const override;

    // IBackendPlugin — conflict presentation
    void    enrichConflictSnapshot(
        Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
        bool isSourceSide) const override;
    QString formatConflictRecordHtml(
        const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const override;
};

} // namespace WildPalms::Memo

#endif // WILDPALMS_MEMO_MEMOBACKENDPLUGIN_H
