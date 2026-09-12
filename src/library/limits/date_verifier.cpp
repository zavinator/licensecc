/*
 * date_verifier.cpp
 *
 *  Created on: Sep 11, 2026
 *      Author: GC
 */

// sntp.h includes <winsock2.h> on windows and must be included before any header that
// pulls in <windows.h>, datatypes.h does.
#include "../sntp/sntp.h"

#include <algorithm>
#include <cmath>
#include <ctime>

#include <licensecc_properties.h>

#include "date_verifier.hpp"
#include "../base/base.h"
#include "../base/logger.h"
#include "../base/string_utils.h"

namespace license {
using namespace std;

bool license_has_date_limits(const FullLicenseInfo& licInfo) {
	return licInfo.m_limits.find(PARAM_EXPIRY_DATE) != licInfo.m_limits.end() ||
		   licInfo.m_limits.find(PARAM_BEGIN_DATE) != licInfo.m_limits.end();
}

LCC_EVENT_TYPE check_clock_offset(bool ntp_reachable, double offset_seconds) {
	if (ntp_reachable) {
		if (fabs(offset_seconds) <= (double)(MAX_ALLOWED_OFFSET_SEC)) {
			return LICENSE_OK;
		}
		LOG_ERROR(
			"The difference between the system clock and the NTP server [%s] is %ld seconds, more than the "
			"%d seconds allowed. The system time can't be trusted.",
			NTP_SERVER_NAME, (long)offset_seconds, (int)(MAX_ALLOWED_OFFSET_SEC));
		return TIME_OUT_OF_SYNC;
	}

#if (NTP_CHECK == NTP_CHECK_REQUIRED)
	LOG_ERROR(
		"The NTP server [%s] is not reachable and NTP_CHECK is set to NTP_CHECK_REQUIRED. The system "
		"time can't be trusted.",
		NTP_SERVER_NAME);
	return TIME_OUT_OF_SYNC;
#else
	LOG_WARN("The NTP server [%s] is not reachable, falling back to the system clock.", NTP_SERVER_NAME);
	return LICENSE_OK;
#endif
}

LCC_EVENT_TYPE sync_system_clock(time_t& out_now) {
	out_now = time(nullptr);

#if (NTP_CHECK != NTP_CHECK_NO)
	time_t server_time = 0;
	time_t reference_time = 0;
	const bool ntp_reachable = sntp::query(NTP_SERVER_NAME, server_time, reference_time);
	// the local time has been captured just before the request was sent, so the offset
	// includes the network round trip.
	const double offset_seconds = ntp_reachable ? difftime(reference_time, server_time) : 0.0;

	const LCC_EVENT_TYPE result = check_clock_offset(ntp_reachable, offset_seconds);
	if (ntp_reachable) {
		out_now = server_time;
		LOG_DEBUG("NTP server [%s] answered, the system clock is off by %ld seconds.", NTP_SERVER_NAME,
				  (long)offset_seconds);
	}
	return result;
#else
	return LICENSE_OK;
#endif
}

LCC_EVENT_TYPE verify_date_at(const FullLicenseInfo& licInfo, LicenseInfo& out, time_t now) {
	bool is_valid = true;

	const auto expiry = licInfo.m_limits.find(PARAM_EXPIRY_DATE);
	if (expiry != licInfo.m_limits.end()) {
		mstrlcpy(out.expiry_date, expiry->second.c_str(), sizeof(out.expiry_date));
		out.has_expiry = true;
		time_t expiry_seconds;
		if (seconds_from_epoch(expiry->second, expiry_seconds)) {
			if (expiry_seconds < now) {
				is_valid = false;
			}
			const double secs = difftime(expiry_seconds, now);
			out.days_left = max((int)round(secs / (60 * 60 * 24)), 0);
		} else {
			out.days_left = 0;
			is_valid = false;
		}
	} else {
		out.has_expiry = false;
		out.days_left = 9999;
	}

	const auto start_date = licInfo.m_limits.find(PARAM_BEGIN_DATE);
	if (start_date != licInfo.m_limits.end()) {
		time_t start_seconds;
		if (!seconds_from_epoch(start_date->second, start_seconds) || start_seconds > now) {
			is_valid = false;
		}
	}

	return is_valid ? LICENSE_OK : PRODUCT_EXPIRED;
}

LCC_EVENT_TYPE verify_date(const FullLicenseInfo& licInfo, LicenseInfo& out) {
	time_t now = time(nullptr);
	if (license_has_date_limits(licInfo)) {
		const LCC_EVENT_TYPE clock_result = sync_system_clock(now);
		if (clock_result != LICENSE_OK) {
			return clock_result;
		}
	}
	return verify_date_at(licInfo, out, now);
}

} /* namespace license */
