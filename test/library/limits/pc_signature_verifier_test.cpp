#define BOOST_TEST_MODULE test_pc_signature_verifier

#include <boost/test/unit_test.hpp>

#include <licensecc_properties.h>
#include <licensecc/datatypes.h>

#include "../../../src/library/LicenseParser.hpp"
#include "../../../src/library/limits/limit_verifiers.hpp"
#include "../../../src/library/hw_identifier/hw_identifier.hpp"
#include "../../../src/library/hw_identifier/hw_identifier_facade.hpp"
#include "../../../src/library/os/os.h"

#include <array>
#include <cstdlib>
#include <stdexcept>

namespace license {
namespace test {

using namespace std;

static FullLicenseInfo make_license() {
	FullLicenseInfo lic;
	lic.source = "test_license.lic";
	lic.m_project = "PRODUCT";
	lic.license_signature = "signature";
	return lic;
}

BOOST_AUTO_TEST_CASE(missing_client_signature_respects_required_strategy) {
	FullLicenseInfo lic = make_license();
	LicenseInfo out;

	const LCC_EVENT_TYPE result = verify_pc_signature(lic, out);

#ifdef LCC_REQUIRED_HW_STRATEGY
    BOOST_CHECK_EQUAL(result, IDENTIFIERS_MISMATCH);
#else
    BOOST_CHECK_EQUAL(result, LICENSE_OK);
#endif
	BOOST_CHECK_EQUAL(out.linked_to_pc, false);
}

BOOST_AUTO_TEST_CASE(invalid_client_signature_is_error) {
	FullLicenseInfo lic = make_license();
	lic.m_limits[PARAM_CLIENT_SIGNATURE] = "0000-0000-0000-0000";
	LicenseInfo out;

	const LCC_EVENT_TYPE result = verify_pc_signature(lic, out);

	BOOST_CHECK_EQUAL(result, IDENTIFIERS_MISMATCH);
	BOOST_CHECK_EQUAL(out.linked_to_pc, true);
}

#ifdef LCC_REQUIRED_HW_STRATEGY

class StrategyEnvironment {
private:
    bool m_wasSet;
    std::string m_previousValue;

public:
    explicit StrategyEnvironment(const char* value)
    {
        const char* previousValue = std::getenv(LCC_IDENTIFICATION_STRATEGY_ENV_VAR);
        m_wasSet = previousValue != nullptr;
        if (m_wasSet)
        {
            m_previousValue = previousValue;
        }
        if (value != nullptr)
        {
            SETENV(LCC_IDENTIFICATION_STRATEGY_ENV_VAR, value);
        }
        else
        {
            UNSETENV(LCC_IDENTIFICATION_STRATEGY_ENV_VAR);
        }
    }

    ~StrategyEnvironment()
    {
        if (m_wasSet)
        {
            SETENV(LCC_IDENTIFICATION_STRATEGY_ENV_VAR, m_previousValue.c_str());
        }
        else
        {
            UNSETENV(LCC_IDENTIFICATION_STRATEGY_ENV_VAR);
        }
    }
};

BOOST_AUTO_TEST_CASE(required_strategy_generation_and_verification)
{
    using hw_identifier::HwIdentifier;
    using hw_identifier::HwIdentifierFacade;

    std::string expectedId;
    try
    {
        expectedId = HwIdentifierFacade::generate_user_pc_signature(LCC_REQUIRED_HW_STRATEGY);
    }
    catch (const std::logic_error&)
    {
        BOOST_TEST_MESSAGE("Required hardware is unavailable; checking failure instead of identifier matching");
        StrategyEnvironment environment(nullptr);
        BOOST_CHECK_THROW(HwIdentifierFacade::generate_user_pc_signature(STRATEGY_DEFAULT), std::logic_error);
        FullLicenseInfo license = make_license();
        license.m_limits[PARAM_CLIENT_SIGNATURE] = "unavailable";
        LicenseInfo out;
        BOOST_CHECK_EQUAL(verify_pc_signature(license, out), IDENTIFIER_NOT_AVAILABLE);
        return;
    }

    FullLicenseInfo license = make_license();
    LicenseInfo out;
    // A default request must agree with the verifier regardless of an environment override.
    const char* environmentValues[] = {nullptr, "0", "2", "3"};
    for (const char* value : environmentValues)
    {
        StrategyEnvironment environment(value);
        const std::string generatedId = HwIdentifierFacade::generate_user_pc_signature(STRATEGY_DEFAULT);
        BOOST_CHECK_EQUAL(generatedId, expectedId);
        license.m_limits[PARAM_CLIENT_SIGNATURE] = generatedId;
        BOOST_CHECK_EQUAL(verify_pc_signature(license, out), LICENSE_OK);
        BOOST_CHECK(out.linked_to_pc);
    }

    HwIdentifier environmentId(expectedId);
    environmentId.set_use_environment_var(true);
    BOOST_REQUIRE(environmentId.print() != expectedId);
    license.m_limits[PARAM_CLIENT_SIGNATURE] = environmentId.print();
    BOOST_CHECK_EQUAL(verify_pc_signature(license, out), LICENSE_OK);

    auto differentPayload = std::array<uint8_t, HW_IDENTIFIER_PROPRIETARY_DATA>{};
    const auto& originalData = environmentId.get_data();
    for (size_t i = 0; i < differentPayload.size(); ++i)
    {
        differentPayload[i] = originalData[i + 1];
    }
    differentPayload[0] ^= 1;
    environmentId.set_data(differentPayload);
    license.m_limits[PARAM_CLIENT_SIGNATURE] = environmentId.print();
    BOOST_CHECK_EQUAL(verify_pc_signature(license, out), IDENTIFIERS_MISMATCH);

    HwIdentifier otherStrategy(expectedId);
    otherStrategy.set_identification_strategy(LCC_REQUIRED_HW_STRATEGY == STRATEGY_DISK
                                                 ? STRATEGY_ETHERNET : STRATEGY_DISK);
    license.m_limits[PARAM_CLIENT_SIGNATURE] = otherStrategy.print();
    BOOST_CHECK_EQUAL(verify_pc_signature(license, out), IDENTIFIERS_MISMATCH);
}

#endif

}  // namespace test
}  // namespace license