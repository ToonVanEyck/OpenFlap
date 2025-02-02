#pragma once

#include <stdint.h>

/**
 * Extracts the major, minor and patch version numbers from a version string.
 *
 * \param[in] version_str The version string to extract the version numbers from.
 * \param[out] major The major version number.
 * \param[out] minor The minor version number.
 * \param[out] patch The patch version number.
 */
void util_extract_version(const char *version_str, uint8_t *major, uint8_t *minor, uint8_t *patch);