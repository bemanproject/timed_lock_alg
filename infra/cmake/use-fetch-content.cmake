cmake_minimum_required(VERSION 3.24)

include(FetchContent)

if(NOT BEMAN_TIMED_LOCK_ALG_LOCKFILE)
    set(BEMAN_TIMED_LOCK_ALG_LOCKFILE
        "lockfile.json"
        CACHE FILEPATH
        "Path to the dependency lockfile for the Beman Timed Lock Algorithms."
    )
endif()

set(BemanTimedLockAlg_projectDir "${CMAKE_CURRENT_LIST_DIR}/../..")
message(TRACE "BemanTimedLockAlg_projectDir=\"${BemanTimedLockAlg_projectDir}\"")

message(TRACE "BEMAN_TIMED_LOCK_ALG_LOCKFILE=\"${BEMAN_TIMED_LOCK_ALG_LOCKFILE}\"")
file(
    REAL_PATH
    "${BEMAN_TIMED_LOCK_ALG_LOCKFILE}"
    BemanTimedLockAlg_lockfile
    BASE_DIRECTORY "${BemanTimedLockAlg_projectDir}"
    EXPAND_TILDE
)
message(DEBUG "Using lockfile: \"${BemanTimedLockAlg_lockfile}\"")

# Force CMake to reconfigure the project if the lockfile changes
set_property(
    DIRECTORY "${BemanTimedLockAlg_projectDir}"
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS "${BemanTimedLockAlg_lockfile}"
)

# For more on the protocol for this function, see:
# https://cmake.org/cmake/help/latest/command/cmake_language.html#provider-commands
function(BemanTimedLockAlg_provideDependency method package_name)
    # Read the lockfile
    file(READ "${BemanTimedLockAlg_lockfile}" BemanTimedLockAlg_rootObj)

    # Get the "dependencies" field and store it in BemanTimedLockAlg_dependenciesObj
    string(
        JSON
        BemanTimedLockAlg_dependenciesObj
        ERROR_VARIABLE BemanTimedLockAlg_error
        GET "${BemanTimedLockAlg_rootObj}"
        "dependencies"
    )
    if(BemanTimedLockAlg_error)
        message(FATAL_ERROR "${BemanTimedLockAlg_lockfile}: ${BemanTimedLockAlg_error}")
    endif()

    # Get the length of the libraries array and store it in BemanTimedLockAlg_dependenciesObj
    string(
        JSON
        BemanTimedLockAlg_numDependencies
        ERROR_VARIABLE BemanTimedLockAlg_error
        LENGTH "${BemanTimedLockAlg_dependenciesObj}"
    )
    if(BemanTimedLockAlg_error)
        message(FATAL_ERROR "${BemanTimedLockAlg_lockfile}: ${BemanTimedLockAlg_error}")
    endif()

    if(BemanTimedLockAlg_numDependencies EQUAL 0)
        return()
    endif()

    # Loop over each dependency object
    math(EXPR BemanTimedLockAlg_maxIndex "${BemanTimedLockAlg_numDependencies} - 1")
    foreach(BemanTimedLockAlg_index RANGE "${BemanTimedLockAlg_maxIndex}")
        set(BemanTimedLockAlg_errorPrefix
            "${BemanTimedLockAlg_lockfile}, dependency ${BemanTimedLockAlg_index}"
        )

        # Get the dependency object at BemanTimedLockAlg_index
        # and store it in BemanTimedLockAlg_depObj
        string(
            JSON
            BemanTimedLockAlg_depObj
            ERROR_VARIABLE BemanTimedLockAlg_error
            GET "${BemanTimedLockAlg_dependenciesObj}"
            "${BemanTimedLockAlg_index}"
        )
        if(BemanTimedLockAlg_error)
            message(
                FATAL_ERROR
                "${BemanTimedLockAlg_errorPrefix}: ${BemanTimedLockAlg_error}"
            )
        endif()

        # Get the "name" field and store it in BemanTimedLockAlg_name
        string(
            JSON
            BemanTimedLockAlg_name
            ERROR_VARIABLE BemanTimedLockAlg_error
            GET "${BemanTimedLockAlg_depObj}"
            "name"
        )
        if(BemanTimedLockAlg_error)
            message(
                FATAL_ERROR
                "${BemanTimedLockAlg_errorPrefix}: ${BemanTimedLockAlg_error}"
            )
        endif()

        # Get the "package_name" field and store it in BemanTimedLockAlg_pkgName
        string(
            JSON
            BemanTimedLockAlg_pkgName
            ERROR_VARIABLE BemanTimedLockAlg_error
            GET "${BemanTimedLockAlg_depObj}"
            "package_name"
        )
        if(BemanTimedLockAlg_error)
            message(
                FATAL_ERROR
                "${BemanTimedLockAlg_errorPrefix}: ${BemanTimedLockAlg_error}"
            )
        endif()

        # Get the "git_repository" field and store it in BemanTimedLockAlg_repo
        string(
            JSON
            BemanTimedLockAlg_repo
            ERROR_VARIABLE BemanTimedLockAlg_error
            GET "${BemanTimedLockAlg_depObj}"
            "git_repository"
        )
        if(BemanTimedLockAlg_error)
            message(
                FATAL_ERROR
                "${BemanTimedLockAlg_errorPrefix}: ${BemanTimedLockAlg_error}"
            )
        endif()

        # Get the "git_tag" field and store it in BemanTimedLockAlg_tag
        string(
            JSON
            BemanTimedLockAlg_tag
            ERROR_VARIABLE BemanTimedLockAlg_error
            GET "${BemanTimedLockAlg_depObj}"
            "git_tag"
        )
        if(BemanTimedLockAlg_error)
            message(
                FATAL_ERROR
                "${BemanTimedLockAlg_errorPrefix}: ${BemanTimedLockAlg_error}"
            )
        endif()

        if(method STREQUAL "FIND_PACKAGE")
            if(package_name STREQUAL BemanTimedLockAlg_pkgName)
                string(
                    APPEND
                    BemanTimedLockAlg_debug
                    "Redirecting find_package calls for ${BemanTimedLockAlg_pkgName} "
                    "to FetchContent logic.\n"
                )
                string(
                    APPEND
                    BemanTimedLockAlg_debug
                    "Fetching ${BemanTimedLockAlg_repo} at "
                    "${BemanTimedLockAlg_tag} according to ${BemanTimedLockAlg_lockfile}."
                )
                message(DEBUG "${BemanTimedLockAlg_debug}")
                FetchContent_Declare(
                    "${BemanTimedLockAlg_name}"
                    GIT_REPOSITORY "${BemanTimedLockAlg_repo}"
                    GIT_TAG "${BemanTimedLockAlg_tag}"
                    EXCLUDE_FROM_ALL
                )
                set(INSTALL_GTEST OFF) # Disable GoogleTest installation
                FetchContent_MakeAvailable("${BemanTimedLockAlg_name}")

                # Important! <PackageName>_FOUND tells CMake that `find_package` is
                # not needed for this package anymore
                set("${BemanTimedLockAlg_pkgName}_FOUND" TRUE PARENT_SCOPE)
            endif()
        endif()
    endforeach()
endfunction()

cmake_language(
    SET_DEPENDENCY_PROVIDER BemanTimedLockAlg_provideDependency
    SUPPORTED_METHODS FIND_PACKAGE
)

# Add this dir to the module path so that `find_package(beman-install-library)` works
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}")
