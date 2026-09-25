/*
 * limit_verifiers.cpp
 *
 *  Created on: Aug 31, 2026
 *      Author: GC
 */

#include <exception>
#include <sstream>

#include "limit_verifiers.hpp"
#include "../base/base.h"
#include "../base/string_utils.h"
#include "../base/logger.h"
#include "../hw_identifier/hw_identifier_facade.hpp"
#include "../hw_identifier/hw_identifier.hpp"
#include "../os/execution_environment.hpp"
#include "../os/signature_verifier.hpp"

namespace license {
using namespace std;

string printForSign(const FullLicenseInfo& licInfo) {
	ostringstream oss;
	oss << toupper_copy(trim_copy(licInfo.m_project));
	for (auto& it : licInfo.m_limits) {
		if (it.first != LICENSE_SIGNATURE) {
			oss << trim_copy(it.first) << trim_copy(it.second);
		}
	}

	LOG_DEBUG("license to sign [%s]", oss.str().c_str());
	return oss.str();
}

LCC_EVENT_TYPE verify_pc_signature(const FullLicenseInfo& licInfo, LicenseInfo& out) {
	const auto client_sig = licInfo.m_limits.find(PARAM_CLIENT_SIGNATURE);
	out.linked_to_pc = (client_sig != licInfo.m_limits.end());
	if (client_sig == licInfo.m_limits.end()) {
#ifdef LCC_REQUIRED_HW_STRATEGY
		return IDENTIFIERS_MISMATCH;
#else
		return LICENSE_OK;
#endif
	}
#ifdef LCC_REQUIRED_HW_STRATEGY
	try {
		const string expected_id = hw_identifier::HwIdentifierFacade::generate_user_pc_signature(LCC_REQUIRED_HW_STRATEGY);
        if (client_sig->second == expected_id)
        {
            return LICENSE_OK;
        }
        // The environment flag records how the strategy was selected, not a different disk.
        hw_identifier::HwIdentifier expectedId(expected_id);
        expectedId.set_use_environment_var(true);
        return client_sig->second == expectedId.print() ? LICENSE_OK : IDENTIFIERS_MISMATCH;
	} catch (const std::exception&) {
		return IDENTIFIER_NOT_AVAILABLE;
	}
#else
	return hw_identifier::HwIdentifierFacade::validate_pc_signature(client_sig->second);
#endif
}

LCC_EVENT_TYPE verify_virtualization(const FullLicenseInfo& licInfo, LicenseInfo& out) {
	const auto virt_type = licInfo.m_limits.find(PARAM_VIRTUALIZATION_TYPE);
	if (virt_type == licInfo.m_limits.end()) {
		return LICENSE_OK;
	}
	const string required = toupper_copy(trim_copy(virt_type->second));
	LCC_API_VIRTUALIZATION_SUMMARY required_summary;
	if (required == "NONE") {
		required_summary = LCC_API_VIRTUALIZATION_SUMMARY::NONE;
	} else if (required == "CONTAINER") {
		required_summary = LCC_API_VIRTUALIZATION_SUMMARY::CONTAINER;
	} else if (required == "VM") {
		required_summary = LCC_API_VIRTUALIZATION_SUMMARY::VM;
	} else {
		LOG_WARN("Unrecognized %s value [%s]", PARAM_VIRTUALIZATION_TYPE, virt_type->second.c_str());
		return LCC_EVENT_TYPE::LCC_INTERNAL_ERROR;
	}
	const os::ExecutionEnvironment exec_env;
	return (exec_env.virtualization() == required_summary) ? LICENSE_OK : IDENTIFIERS_MISMATCH;
}

LCC_EVENT_TYPE verify_signature(const FullLicenseInfo& licInfo, LicenseInfo& out) {
	const string licInfoData(printForSign(licInfo));

	const FUNCTION_RETURN ret = license::os::verify_signature(licInfoData, licInfo.license_signature);

	return (ret == FUNC_RET_OK) ? SIGNATURE_VERIFIED : LICENSE_CORRUPTED;
}

} /* namespace license */
