# SPDX-License-Identifier: AGPL-3.0-only
# The retry decision: whether a failed answer is worth asking again, and how
# long to wait, compiled as classic code because the ported policy it reads
# lives in Agora's headers (see cmake/providers.cmake).
target_sources(pchat_provider PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/retry.cc)
