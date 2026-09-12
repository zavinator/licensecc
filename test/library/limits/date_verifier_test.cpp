#define BOOST_TEST_MODULE test_date_verifier

#include <boost/test/unit_test.hpp>

#include <ctime>

#include <licensecc_properties.h>
#include <licensecc/datatypes.h>

#include "../../../src/library/LicenseParser.hpp"
#include "../../../src/library/base/string_utils.h"
#include "../../../src/library/limits/date_verifier.hpp"

/*
 * date_verifier.hpp exposes only verify_date, the steps that do the real work are declared here,
 * they take the time to compare the license dates with as a parameter: it keeps these unit tests
 * deterministic and independent from the machine clock and from the network. verify_date, that
 * queries the NTP server, is exercised only when it can't perform any network access.
 */
namespace license {

bool license_has_date_limits(const FullLicenseInfo& licInfo);
LCC_EVENT_TYPE check_clock_offset(bool ntp_reachable, double offset_seconds);
LCC_EVENT_TYPE sync_system_clock(time_t& out_now);
LCC_EVENT_TYPE verify_date_at(const FullLicenseInfo& licInfo, LicenseInfo& out, time_t now);

namespace test {

using namespace std;

static FullLicenseInfo make_license() {
	FullLicenseInfo lic;
	lic.source = "test_license.lic";
	lic.m_project = "PRODUCT";
	lic.license_signature = "signature";
	return lic;
}

static FullLicenseInfo make_expired_license() {
	FullLicenseInfo lic = make_license();
	lic.m_limits[PARAM_EXPIRY_DATE] = "2013-10-10";
	return lic;
}

static time_t date_at(const string& date) {
	time_t seconds;
	BOOST_REQUIRE_MESSAGE(seconds_from_epoch(date, seconds), "date [" + date + "] should be parsed");
	return seconds;
}

BOOST_AUTO_TEST_CASE(no_dates_is_valid) {
	FullLicenseInfo lic = make_license();
	LicenseInfo out;

	// no date limit in the license means no NTP query, so verify_date is safe to call here
	const LCC_EVENT_TYPE result = verify_date(lic, out);

	BOOST_CHECK_EQUAL(result, LICENSE_OK);
	BOOST_CHECK_EQUAL(out.has_expiry, false);
	BOOST_CHECK_EQUAL(out.days_left, (unsigned int)9999);
	BOOST_CHECK_EQUAL(license_has_date_limits(lic), false);
}

BOOST_AUTO_TEST_CASE(dates_are_detected) {
	FullLicenseInfo expiry_only = make_license();
	expiry_only.m_limits[PARAM_EXPIRY_DATE] = "2050-10-10";
	BOOST_CHECK_EQUAL(license_has_date_limits(expiry_only), true);

	FullLicenseInfo begin_only = make_license();
	begin_only.m_limits[PARAM_BEGIN_DATE] = "2013-10-10";
	BOOST_CHECK_EQUAL(license_has_date_limits(begin_only), true);

	BOOST_CHECK_EQUAL(license_has_date_limits(make_expired_license()), true);
}

BOOST_AUTO_TEST_CASE(not_expired_is_valid) {
	FullLicenseInfo lic = make_license();
	lic.m_limits[PARAM_EXPIRY_DATE] = "2050-10-10";
	LicenseInfo out;

	const LCC_EVENT_TYPE result = verify_date_at(lic, out, date_at("2026-09-11"));

	BOOST_CHECK_EQUAL(result, LICENSE_OK);
	BOOST_CHECK_EQUAL(out.has_expiry, true);
	BOOST_CHECK_GT(out.days_left, (unsigned int)0);
	BOOST_CHECK_EQUAL(string(out.expiry_date), string("2050-10-10"));
}

BOOST_AUTO_TEST_CASE(days_left_are_counted_from_the_verified_time) {
	FullLicenseInfo lic = make_license();
	lic.m_limits[PARAM_EXPIRY_DATE] = "2050-10-10";
	LicenseInfo out;

	BOOST_REQUIRE_EQUAL(verify_date_at(lic, out, date_at("2050-10-01")), LICENSE_OK);
	const unsigned int days_left = out.days_left;
	BOOST_CHECK_GT(days_left, (unsigned int)0);
	BOOST_CHECK_LT(days_left, (unsigned int)20);

	// one more day of uptime is one day less of license
	BOOST_REQUIRE_EQUAL(verify_date_at(lic, out, date_at("2050-10-02")), LICENSE_OK);
	BOOST_CHECK_LT(out.days_left, days_left);
}

BOOST_AUTO_TEST_CASE(expired_is_error) {
	FullLicenseInfo lic = make_expired_license();
	LicenseInfo out;

	const LCC_EVENT_TYPE result = verify_date_at(lic, out, date_at("2026-09-11"));

	BOOST_CHECK_EQUAL(result, PRODUCT_EXPIRED);
	BOOST_CHECK_EQUAL(out.days_left, (unsigned int)0);
	BOOST_CHECK_EQUAL(out.has_expiry, true);
}

BOOST_AUTO_TEST_CASE(expiring_same_day_is_valid) {
	FullLicenseInfo lic = make_license();
	lic.m_limits[PARAM_EXPIRY_DATE] = "2050-10-10";
	LicenseInfo out;

	BOOST_CHECK_EQUAL(verify_date_at(lic, out, date_at("2050-10-10")), LICENSE_OK);
	BOOST_CHECK_EQUAL(verify_date_at(lic, out, date_at("2050-10-10") + 1), PRODUCT_EXPIRED);
}

BOOST_AUTO_TEST_CASE(future_start_date_is_error) {
	FullLicenseInfo lic = make_license();
	lic.m_limits[PARAM_BEGIN_DATE] = "2050-10-10";
	LicenseInfo out;

	BOOST_CHECK_EQUAL(verify_date_at(lic, out, date_at("2026-09-11")), PRODUCT_EXPIRED);
	BOOST_CHECK_EQUAL(verify_date_at(lic, out, date_at("2050-10-10")), LICENSE_OK);
}

BOOST_AUTO_TEST_CASE(malformed_expiry_is_error) {
	FullLicenseInfo lic = make_license();
	lic.m_limits[PARAM_EXPIRY_DATE] = "not-a-date";
	LicenseInfo out;

	const LCC_EVENT_TYPE result = verify_date_at(lic, out, date_at("2026-09-11"));

	BOOST_CHECK_EQUAL(result, PRODUCT_EXPIRED);
	BOOST_CHECK_EQUAL(out.days_left, (unsigned int)0);
}

BOOST_AUTO_TEST_CASE(a_clock_set_in_the_past_revives_an_expired_license) {
	// this is why the dates are verified against the NTP server time and not the system clock:
	// a license expired in 2013 is valid again for a machine reporting a time before that date.
	FullLicenseInfo lic = make_expired_license();
	LicenseInfo out;

	BOOST_CHECK_EQUAL(verify_date_at(lic, out, date_at("2013-10-09")), LICENSE_OK);
	BOOST_CHECK_EQUAL(verify_date_at(lic, out, date_at("2026-09-11")), PRODUCT_EXPIRED);
}

BOOST_AUTO_TEST_CASE(clock_offset_within_the_limit_is_accepted) {
	BOOST_CHECK_EQUAL(check_clock_offset(true, 0.0), LICENSE_OK);
	BOOST_CHECK_EQUAL(check_clock_offset(true, (double)(MAX_ALLOWED_OFFSET_SEC)-1), LICENSE_OK);
	// a clock running fast is off by a negative amount, it is accepted too
	BOOST_CHECK_EQUAL(check_clock_offset(true, -((double)(MAX_ALLOWED_OFFSET_SEC)-1)), LICENSE_OK);
	BOOST_CHECK_EQUAL(check_clock_offset(true, ((double)(MAX_ALLOWED_OFFSET_SEC)) * -1), LICENSE_OK);
}

BOOST_AUTO_TEST_CASE(clock_offset_over_the_limit_is_rejected) {
	const double over_the_limit = (double)(MAX_ALLOWED_OFFSET_SEC) + 1;

	BOOST_CHECK_EQUAL(check_clock_offset(true, over_the_limit), TIME_OUT_OF_SYNC);
	BOOST_CHECK_EQUAL(check_clock_offset(true, -over_the_limit), TIME_OUT_OF_SYNC);
	// a clock set one day in the past is the usual way an expired license is revived
	BOOST_CHECK_EQUAL(check_clock_offset(true, 86400.0), TIME_OUT_OF_SYNC);
}

BOOST_AUTO_TEST_CASE(ntp_check_is_configured) {
	BOOST_CHECK_MESSAGE(NTP_CHECK == NTP_CHECK_NO || NTP_CHECK == NTP_CHECK_OPTIONAL || NTP_CHECK == NTP_CHECK_REQUIRED,
						"NTP_CHECK must be one of NTP_CHECK_NO, NTP_CHECK_OPTIONAL, NTP_CHECK_REQUIRED");
}

#if (NTP_CHECK == NTP_CHECK_REQUIRED)
BOOST_AUTO_TEST_CASE(missing_ntp_server_is_an_error) {
	BOOST_CHECK_EQUAL(check_clock_offset(false, 0.0), TIME_OUT_OF_SYNC);
	BOOST_CHECK_EQUAL(check_clock_offset(false, (double)(MAX_ALLOWED_OFFSET_SEC)), TIME_OUT_OF_SYNC);
}
#else
BOOST_AUTO_TEST_CASE(missing_ntp_server_falls_back_to_the_system_clock) {
	BOOST_CHECK_EQUAL(check_clock_offset(false, 0.0), LICENSE_OK);
	BOOST_CHECK_EQUAL(check_clock_offset(false, 86400.0 * 365), LICENSE_OK);
}
#endif

}  // namespace test
}  // namespace license
