# SPDX-License-Identifier: AGPL-3.0-only
# The http policy's own parts: what a client is built with and what this
# program refuses to send over a wire that is not encrypted, compiled as
# classic code and reached from the composed modules through the one header in
# this folder (see cmake/providers.cmake).
target_sources(pchat_provider PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wire.cpp)
