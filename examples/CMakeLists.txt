# SPDX-License-Identifier: MIT

set(ALL_EXAMPLES try_lock_a_bunch)

message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.timed_lock_alg.examples.${example})
    target_sources(
        beman.timed_lock_alg.examples.${example}
        PRIVATE ${example}.cpp
    )
    target_link_libraries(
        beman.timed_lock_alg.examples.${example}
        PRIVATE beman::timed_lock_alg
    )
endforeach()
