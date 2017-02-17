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

#include <iostream>
#include <vintf/parse_xml.h>
#include <vintf/parse_string.h>
#include <vintf/KernelInfo.h>
#include <vintf/VendorManifest.h>

int main(int, char **) {
    using namespace ::android::vintf;

    const VendorManifest *vm = VendorManifest::Get();
    if (vm != nullptr)
        std::cout << gVendorManifestConverter(*vm);

    std::cout << std::endl;
    const KernelInfo *ki = KernelInfo::Get();
    if (ki != nullptr)
        std::cout << dump(*ki);
    std::cout << std::endl;
}