/*
 * date_verifier.hpp
 *
 *  Created on: Sep 11, 2026
 *      Author: GC
 */

#ifndef SRC_LIBRARY_LIMITS_DATE_VERIFIER_HPP_
#define SRC_LIBRARY_LIMITS_DATE_VERIFIER_HPP_

#include <licensecc/datatypes_cpp.hpp>

namespace license {

/**
 * Verifies the license begin/expiry dates and populates the date fields of `LicenseInfo`.
 * If the license declares a date limit the system clock is checked against the NTP server declared
 * by `LCC_NTP_CHECK`/`LCC_NTP_SERVER_NAME` in `licensecc_properties.h`, and the dates are verified against
 * the server time. Licenses without date limits are verified against the system clock and no
 * network access is performed.
 */
LCC_EVENT_TYPE verify_date(const FullLicenseInfo& licInfo, LicenseInfo& out);

} /* namespace license */

#endif /* SRC_LIBRARY_LIMITS_DATE_VERIFIER_HPP_ */
