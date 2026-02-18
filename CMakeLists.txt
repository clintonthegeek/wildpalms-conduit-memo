kcoreaddons_add_plugin(wildpalms_memo
    SOURCES
        memoconduit.cpp
        memoconduit.h
        memomapper.cpp
        memomapper.h
        memoview.cpp
        memoview.h
    INSTALL_NAMESPACE "wildpalms/conduits"
)

target_link_libraries(wildpalms_memo
    WildPalmsCore
    KF6::CoreAddons
    KF6::I18n
    KF6::WidgetsAddons
    Qt::Widgets
)
