
/*
 * LicenseVerifier_test.cpp
 *
 *  Created on: Nov 17, 2019
 *      Author: GC
 */
#define BOOST_TEST_MODULE test_signature_verifier

#include <boost/test/unit_test.hpp>
#include <licensecc_properties_test.h>
#include <licensecc_properties.h>

#include "../../src/library/os/signature_verifier.hpp"
#include "generate-license.h"

#include <string>
#include <vector>

namespace license {
namespace test {
using namespace std;

BOOST_AUTO_TEST_CASE(verify_signature_ok) {
	const string test_data("test_data");
	const string signature = sign_data(test_data, string("verify_signature"));

	FUNCTION_RETURN result = license::os::verify_signature(test_data, signature);
	BOOST_CHECK_MESSAGE(result == FUNC_RET_OK, "signature verified");
}

BOOST_AUTO_TEST_CASE(verify_signature_data_mismatch) {
	const string test_data("test_data");
	const string signature = sign_data(test_data, string("verify_signature"));

	FUNCTION_RETURN result = license::os::verify_signature(string("other data"), signature);
	BOOST_CHECK_MESSAGE(result == FUNC_RET_ERROR, "signature NOT verified");
}

BOOST_AUTO_TEST_CASE(verify_signature_modified) {
	const string test_data("test_data");
	string signature = sign_data(test_data, string("verify_signature"));
	signature[2] = signature[2] + 1;
	FUNCTION_RETURN result = license::os::verify_signature(test_data, signature);
	BOOST_CHECK_MESSAGE(result == FUNC_RET_ERROR, "signature NOT verified");
}

BOOST_AUTO_TEST_CASE(verify_signature_malformed)
{
    const std::vector<std::string> malformedSignatures = {
        "", "A", "ab", "abc", "=", "==", "===", "====", "A=", "A==", "A===",
        "AA=A", "AAAA=", "!!!!", std::string(4, '\xff'), std::string("AA\0A", 4)
    };
    for (size_t i = 0; i < malformedSignatures.size(); ++i)
    {
        BOOST_TEST_CONTEXT("Malformed signature case " << i)
        {
            BOOST_CHECK_EQUAL(os::verify_signature("test_data", malformedSignatures[i]), FUNC_RET_ERROR);
        }
    }
}

BOOST_AUTO_TEST_CASE(verify_signature_truncated)
{
    const std::string data = "test_data";
    const std::string signature = sign_data(data, "verify_signature_truncated");
    // Remove actual signature data, not only optional padding or whitespace.
    const size_t dataLength = signature.find_last_not_of("=\r\n \t") + 1;
    BOOST_REQUIRE(dataLength > 4);
    for (size_t length = 0; length < dataLength; ++length)
    {
        BOOST_TEST_CONTEXT("Truncated signature length " << length)
        {
            BOOST_CHECK_EQUAL(os::verify_signature(data, signature.substr(0, length)), FUNC_RET_ERROR);
        }
    }
}

}  // namespace test

} /* namespace license */
