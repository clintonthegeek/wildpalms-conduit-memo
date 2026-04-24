option(WILDPALMS_MEMO_PLUGIN_V2 "Build the new IBackendPlugin-based Memo plugin" ON)

if (WILDPALMS_MEMO_PLUGIN_V2)
    kcoreaddons_add_plugin(wildpalms_memo_v2
        SOURCES
            memobackendplugin.cpp  memobackendplugin.h
            memoblobbackend.cpp    memoblobbackend.h
            memomarkdown.cpp       memomarkdown.h
            memoview.cpp           memoview.h
        INSTALL_NAMESPACE "wildpalms/plugins"
    )
    target_include_directories(wildpalms_memo_v2
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(wildpalms_memo_v2
        PRIVATE
            WildPalmsCore
            WildPalmsPalmSync
            WildPalmsPalmCodecs
            WildPalmsPalmCalendar
            KF6::CoreAddons
            KF6::I18n
            KF6::WidgetsAddons
            Qt::Widgets
            Kalburator::Sync
    )
else ()
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
endif ()
