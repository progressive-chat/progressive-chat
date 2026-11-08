# SPDX-License-Identifier: AGPL-3.0-only
# The gemini wire's own parts: what decodes its frames, compiled as classic
# code and reached from the composed modules through the one header in this
# folder (see cmake/providers.cmake).
target_sources(pchat_provider PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wire.cpp)
