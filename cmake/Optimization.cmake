include(CheckIPOSupported)

function(chess_enable_release_ipo)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
        foreach(target IN ITEMS
                chess_core chess_search chess_training chess_protocol
                chess_engine train_genetic self_play fen_eval
                build_opening_book compare_models search_benchmark chess_tests)
            if(TARGET "${target}")
                set_property(TARGET "${target}" PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
                set_property(TARGET "${target}" PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
            endif()
        endforeach()
    else()
        message(STATUS "IPO/LTO unavailable: ${ipo_error}")
    endif()
endfunction()
