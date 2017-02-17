/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Convert objects from and to strings.

#include "parse_string.h"
#include <android-base/parseint.h>

namespace android {
using base::ParseUint;

namespace vintf {

static const std::string kRequired("required");
static const std::string kOptional("optional");
static const std::string kConfigPrefix("CONFIG_");

std::vector<std::string> SplitString(const std::string &s, char c) {
    std::vector<std::string> components;

    size_t startPos = 0;
    size_t matchPos;
    while ((matchPos = s.find(c, startPos)) != std::string::npos) {
        components.push_back(s.substr(startPos, matchPos - startPos));
        startPos = matchPos + 1;
    }

    if (startPos <= s.length()) {
        components.push_back(s.substr(startPos));
    }
    return components;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> objs) {
    bool first = true;
    for (const T &v : objs) {
        if (!first) {
            os << ",";
        }
        os << v;
        first = false;
    }
    return os;
}

template <typename T>
bool parse(const std::string &s, std::vector<T> *objs) {
    std::vector<std::string> v = SplitString(s, ',');
    objs->resize(v.size());
    size_t idx = 0;
    for (const auto &item : v) {
        T ver;
        if (!parse(item, &ver)) {
            return false;
        }
        objs->at(idx++) = ver;
    }
    return true;
}

template<typename E, typename Array>
bool parseEnum(const std::string &s, E *e, const Array &strings) {
    for (size_t i = 0; i < strings.size(); ++i) {
        if (s == strings.at(i)) {
            *e = static_cast<E>(i);
            return true;
        }
    }
    return false;
}

bool parse(const std::string &s, HalFormat *hf) {
    return parseEnum(s, hf, gHalFormatStrings);
}

std::ostream &operator<<(std::ostream &os, HalFormat hf) {
    return os << gHalFormatStrings.at(static_cast<size_t>(hf));
}

bool parse(const std::string &s, ImplLevel *il) {
    return parseEnum(s, il, gImplLevelStrings);
}

std::ostream &operator<<(std::ostream &os, ImplLevel il) {
    return os << gImplLevelStrings.at(static_cast<size_t>(il));
}

bool parse(const std::string &s, Transport *tr) {
    return parseEnum(s, tr, gTransportStrings);
}

std::ostream &operator<<(std::ostream &os, Transport tr) {
    return os << gTransportStrings.at(static_cast<size_t>(tr));
}

bool parse(const std::string &s, KernelConfigType *kc) {
    return parseEnum(s, kc, gKernelConfigTypeStrings);
}

std::ostream &operator<<(std::ostream &os, KernelConfigType kc) {
    return os << gKernelConfigTypeStrings.at(static_cast<size_t>(kc));
}

bool parse(const std::string &s, Tristate *kc) {
    return parseEnum(s, kc, gTristateStrings);
}

std::ostream &operator<<(std::ostream &os, Tristate kc) {
    return os << gTristateStrings.at(static_cast<size_t>(kc));
}

std::ostream &operator<<(std::ostream &os, const KernelConfigTypedValue &kctv) {
    switch (kctv.mType) {
        case KernelConfigType::STRING:
            return os << kctv.mStringValue;
        case KernelConfigType::INTEGER:
            return os << to_string(kctv.mIntegerValue);
        case KernelConfigType::RANGE:
            return os << to_string(kctv.mRangeValue.first) << "-"
                      << to_string(kctv.mRangeValue.second);
        case KernelConfigType::TRISTATE:
            return os << to_string(kctv.mTristateValue);
    }
}

// Notice that strtoull is used even though KernelConfigIntValue is signed int64_t,
// because strtoull can accept negative values as well.
// Notice that according to man strtoul, strtoull can actually accept
// -2^64 + 1 to 2^64 - 1, with the 65th bit truncated.
// ParseInt / ParseUint are not used because they do not handle signed hex very well.
template <typename T>
bool parseKernelConfigIntHelper(const std::string &s, T *i) {
    char *end;
    errno = 0;
    unsigned long long int ulli = strtoull(s.c_str(), &end, 0 /* base */);
    // It is implementation defined that what value will be returned by strtoull
    // in the error case, so we are checking errno directly here.
    if (errno == 0 && s.c_str() != end && *end == '\0') {
        *i = static_cast<T>(ulli);
        return true;
    }
    return false;
}

bool parseKernelConfigInt(const std::string &s, int64_t *i) {
    return parseKernelConfigIntHelper(s, i);
}

bool parseKernelConfigInt(const std::string &s, uint64_t *i) {
    return parseKernelConfigIntHelper(s, i);
}

bool parseRange(const std::string &s, KernelConfigRangeValue *range) {
    auto pos = s.find('-');
    if (pos == std::string::npos) {
        return false;
    }
    return parseKernelConfigInt(s.substr(0, pos),  &range->first)
        && parseKernelConfigInt(s.substr(pos + 1), &range->second);
}

bool parse(const std::string &s, KernelConfigKey *key) {
    *key = s;
    return true;
}

bool parseKernelConfigValue(const std::string &s, KernelConfigTypedValue *kctv) {
    switch (kctv->mType) {
        case KernelConfigType::STRING:
            kctv->mStringValue = s;
            return true;
        case KernelConfigType::INTEGER:
            return parseKernelConfigInt(s, &kctv->mIntegerValue);
        case KernelConfigType::RANGE:
            return parseRange(s, &kctv->mRangeValue);
        case KernelConfigType::TRISTATE:
            return parse(s, &kctv->mTristateValue);
    }
}

bool parse(const std::string &s, Version *ver) {
    std::vector<std::string> v = SplitString(s, '.');
    if (v.size() != 2) {
        return false;
    }
    size_t major, minor;
    if (!ParseUint(v[0], &major)) {
        return false;
    }
    if (!ParseUint(v[1], &minor)) {
        return false;
    }
    *ver = Version(major, minor);
    return true;
}

std::ostream &operator<<(std::ostream &os, const Version &ver) {
    return os << ver.majorVer << "." << ver.minorVer;
}

bool parse(const std::string &s, VersionRange *vr) {
    std::vector<std::string> v = SplitString(s, '-');
    if (v.size() != 1 && v.size() != 2) {
        return false;
    }
    Version minVer;
    if (!parse(v[0], &minVer)) {
        return false;
    }
    if (v.size() == 1) {
        *vr = VersionRange(minVer.majorVer, minVer.minorVer);
    } else {
        size_t maxMinor;
        if (!ParseUint(v[1], &maxMinor)) {
            return false;
        }
        *vr = VersionRange(minVer.majorVer, minVer.minorVer, maxMinor);
    }
    return true;
}

std::ostream &operator<<(std::ostream &os, const VersionRange &vr) {
    if (vr.isSingleVersion()) {
        return os << vr.minVer();
    }
    return os << vr.minVer() << "-" << vr.maxMinor;
}

bool parse(const std::string &s, KernelVersion *kernelVersion) {
    std::vector<std::string> v = SplitString(s, '.');
    if (v.size() != 3) {
        return false;
    }
    size_t version, major, minor;
    if (!ParseUint(v[0], &version)) {
        return false;
    }
    if (!ParseUint(v[1], &major)) {
        return false;
    }
    if (!ParseUint(v[2], &minor)) {
        return false;
    }
    *kernelVersion = KernelVersion(version, major, minor);
    return true;
}

std::ostream &operator<<(std::ostream &os, const KernelVersion &ver) {
    return os << ver.version << "." << ver.majorRev << "." << ver.minorRev;
}

bool parse(const std::string &s, ManifestHal *hal) {
    std::vector<std::string> v = SplitString(s, '/');
    if (v.size() != 5) {
        return false;
    }
    if (!parse(v[0], &hal->format)) {
        return false;
    }
    hal->name = v[1];
    if (!parse(v[2], &hal->transport)) {
        return false;
    }
    if (!parse(v[3], &hal->impl.implLevel)) {
        return false;
    }
    hal->impl.impl = v[4];
    if (!parse(v[5], &hal->versions)) {
        return false;
    }
    return hal->isValid();
}

std::ostream &operator<<(std::ostream &os, const ManifestHal &hal) {
    return os << hal.format << "/"
              << hal.name << "/"
              << hal.transport << "/"
              << hal.impl.implLevel << "/"
              << hal.impl.impl << "/"
              << hal.versions;
}

bool parse(const std::string &s, MatrixHal *req) {
    std::vector<std::string> v = SplitString(s, '/');
    if (v.size() != 4) {
        return false;
    }
    if (!parse(v[0], &req->format)) {
        return false;
    }
    req->name = v[1];
    if (!parse(v[2], &req->versionRanges)) {
        return false;
    }
    if (v[3] != kRequired || v[3] != kOptional) {
        return false;
    }
    req->optional = (v[3] == kOptional);
    return true;
}

std::ostream &operator<<(std::ostream &os, const MatrixHal &req) {
    return os << req.format << "/"
              << req.name << "/"
              << req.versionRanges << "/"
              << (req.optional ? kOptional : kRequired);
}


std::ostream &operator<<(std::ostream &os, KernelSepolicyVersion ksv){
    return os << ksv.value;
}

bool parse(const std::string &s, KernelSepolicyVersion *ksv){
    return ParseUint(s, &ksv->value);
}

std::ostream &operator<<(std::ostream &os, const SepolicyVersion &sv){
    return os << sv.minVer << "-" << sv.maxVer;
}

bool parse(const std::string &s, SepolicyVersion *sv){
    std::vector<std::string> v = SplitString(s, '-');
    if (v.size() != 2) {
        return false;
    }
    if (!ParseUint(v[0], &sv->minVer)) {
        return false;
    }
    if (!ParseUint(v[1], &sv->maxVer)) {
        return false;
    }
    return true;
}

std::string dump(const VendorManifest &vm) {
    std::ostringstream oss;
    bool first = true;
    for (const auto &hal : vm.getHals()) {
        if (!first) {
            oss << ":";
        }
        oss << hal;
        first = false;
    }
    return oss.str();
}

std::string dump(const KernelInfo &ki) {
    std::ostringstream oss;

    oss << "kernel = "
        << ki.osName() << "/"
        << ki.nodeName() << "/"
        << ki.osRelease() << "/"
        << ki.osVersion() << "/"
        << ki.hardwareId() << ";"
        << "kernelSepolicyVersion = " << ki.kernelSepolicyVersion() << ";"
        << "#CONFIG's loaded = " << ki.mKernelConfigs.size() << ";\n";
    for (const auto &pair : ki.mKernelConfigs) {
        oss << pair.first << "=" << pair.second << "\n";
    }

    return oss.str();
}

} // namespace vintf
} // namespace android