add_library(wildpalms_memo_static STATIC
    memobackendplugin.cpp  memobackendplugin.h
    memoblobbackend.cpp    memoblobbackend.h
    memomarkdown.cpp       memomarkdown.h
    memoview.cpp           memoview.h
)
target_include_directories(wildpalms_memo_static
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
# Prepend libkalburator's sync dir so #include "syncbackend.h" resolves
# to Kalburator::Sync::SyncBackend (not the legacy WildPalms one).
target_include_directories(wildpalms_memo_static BEFORE
    PRIVATE
        $<TARGET_PROPERTY:Kalburator::Sync,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(wildpalms_memo_static
    PUBLIC
        Kalburator::Sync
    PRIVATE
        WildPalmsCore
        WildPalmsPalmSync
        WildPalmsPalmCodecs
        WildPalmsPalmCalendar
        KF6::I18n
        KF6::WidgetsAddons
        Qt::Widgets
)
