// Vita3K emulator project
// Copyright (C) 2023 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <regex>
#include <regmgr/functions.h>

#include <util/log.h>
#include <util/string_utils.h>

namespace regmgr {

constexpr std::array<unsigned char, 16> xorKey = { 0x89, 0xFA, 0x95, 0x48, 0xCB, 0x6D, 0x77, 0x9D, 0xA2, 0x25, 0x34, 0xFD, 0xA9, 0x35, 0x59, 0x6E };

static std::string decryptRegistryFile(const fs::path reg_path) {
    std::ifstream file(reg_path.string(), std::ios::binary);
    if (!file.is_open()) {
        LOG_WARN("Error while opening file: {}, install firmware for solve this!", reg_path.string());
        return {};
    }

    // Read the data from the file encrypted
    std::vector<uint8_t> encryptedData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Check if file is empty
    if (encryptedData.empty()) {
        LOG_DEBUG("File is empty: {}", reg_path.string());
        return {};
    }

    // Remove the header of the file (138 bytes)
    encryptedData.erase(encryptedData.begin(), encryptedData.begin() + 138);

    // Decrypt the data with the xor key
    for (size_t i = 0; i < encryptedData.size(); i++) {
        encryptedData[i] ^= xorKey[i & 0xF];
    }

    // Convert the data to a string
    std::string res;
    for (const auto &byte : encryptedData) {
        res += byte;
    }

    return res;
}

struct RegValue {
    std::string name;
    uint32_t size;
    std::string value;
};

static std::vector<std::string> reg_category_template;
static std::map<std::string, std::vector<RegValue>> reg_template;

void init_reg_template(RegMgrState &regmgr, const std::string &reg) {
    reg_category_template.clear();
    reg_template.clear();

    std::map<int, std::string> reg_map;

    std::istringstream iss(reg);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;

        // Found the base register
        if (line.find("[BASE") != std::string::npos) {
            // Register the names with their numbers
            while (std::getline(iss, line) && (line != "[REG-BAS")) {
                if (line.empty())
                    continue;

                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    const auto num = string_utils::stoi_def(line.substr(0, pos));
                    const auto name = line.substr(pos + 1);
                    reg_map[num] = name;
                }
            }
        }

        if (line == "[REG-BAS") {
            // Register the base register
            while (std::getline(iss, line) && (line != ("[REG-J1"))) {
                if (line.empty())
                    continue;

                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string entry = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);

                    std::string name = "/";
                    std::istringstream f(entry);
                    std::string s;
                    while (std::getline(f, s, '/')) {
                        if (!s.empty())
                            name += reg_map[string_utils::stoi_def(s)];
                    }

                    const auto last_slash_pos = name.find_last_of('/') + 1;
                    const auto valueName = name.substr(last_slash_pos);
                    const auto category = name.substr(0, last_slash_pos);

                    std::istringstream v(value);
                    std::vector<std::string> values;
                    while (std::getline(v, s, ':')) {
                        if (!s.empty())
                            values.push_back(s);
                    }

                    const auto add_category_template = [&](const std::string &category) {
                        if (std::find(reg_category_template.begin(), reg_category_template.end(), category) == reg_category_template.end())
                            reg_category_template.push_back(category);
                    };

                    const auto valueSize = static_cast<uint32_t>(string_utils::stoi_def(values[1]));
                    const std::regex categoryRangePattern(R"((.*\/)([0-9]{2})-([0-9]{2})(\/.*))");
                    std::smatch matches;
                    if (std::regex_search(category, matches, categoryRangePattern)) {
                        const auto cat_begin = matches[1].str();
                        const auto firstNum = string_utils::stoi_def(matches[2].str());
                        const auto secondNum = string_utils::stoi_def(matches[3].str());
                        const auto cat_end = matches[4].str();

                        for (int i = firstNum; i <= secondNum; i++) {
                            const std::string categoryName = fmt::format("{}{:0>2d}{}", cat_begin, i, cat_end);
                            reg_template[categoryName].push_back({ valueName, valueSize, values.back() });
                            add_category_template(categoryName);
                        }
                    } else {
                        reg_template[category].push_back({ valueName, valueSize, values.back() });
                        add_category_template(category);
                    }
                }
            }
        }
    }
}

static constexpr uint32_t spaceSize = 32;
static uint32_t get_space_size(const uint32_t str_size) {
    const uint32_t line = 16;

    return spaceSize > str_size ? spaceSize - str_size : ((str_size / line) + 1) * line - str_size;
}

static bool load_system_dreg(RegMgrState &regmgr) {
    fs::ifstream file(regmgr.system_dreg_path, std::ios::in | std::ios::binary);
    if (file.is_open()) {
        char buf = 0;

        // Skip the space
        for (uint32_t i = 0; i < (spaceSize + 1); i++) {
            file.read(&buf, 1);
        }

        for (const auto &cat : reg_category_template) {
            // Read the category
            const uint32_t cat_size = static_cast<uint32_t>(cat.size());
            std::vector<char> category(cat_size);
            file.read(category.data(), cat_size);
            const auto category_str = std::string(category.begin(), category.end());
            if (category_str != cat) {
                LOG_ERROR("Invalid category: {}, expected: {}", category_str, cat);
                return false;
            }

            // Skip the space
            const auto diff_cat = get_space_size(cat_size);
            for (uint32_t i = 0; i < diff_cat; i++) {
                file.read(&buf, 1);
            }

            for (const auto &entry : reg_template[cat]) {
                // Read the name
                const uint32_t name_size = static_cast<uint32_t>(entry.name.size());
                std::vector<char> name(name_size);
                file.read(name.data(), name_size);
                const auto name_str = std::string(name.begin(), name.end());
                if (name_str != entry.name) {
                    LOG_ERROR("Invalid entry name: {}, expected: {}, in category: {}", name_str, entry.name, cat);
                    return false;
                }

                // Skip the space
                const auto diff_name = get_space_size(name_size + 1);
                for (uint32_t i = 0; i < diff_name; i++) {
                    file.read(&buf, 1);
                }

                // Read the value
                const uint32_t value_size = entry.size;
                auto &value = regmgr.system_dreg[cat][entry.name];
                value.resize(value_size);
                file.read(value.data(), value_size);

                // Skip the space
                const auto diff_value = get_space_size(value_size);
                for (uint32_t i = 0; i < (diff_value + 1); i++) {
                    file.read(&buf, 1);
                }
            }
        }

        file.close();
    }

    return !regmgr.system_dreg.empty();
}

static void save_system_dreg(RegMgrState &regmgr) {
    fs::ofstream file(regmgr.system_dreg_path, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        const char buf = 0;

        // Write the space
        for (uint32_t i = 0; i < (spaceSize + 1); i++) {
            file.write(&buf, 1);
        }

        // Write each category
        for (const auto &cat : reg_category_template) {
            // Write the category
            const uint32_t cat_size = static_cast<uint32_t>(cat.size());
            std::vector<char> category(cat_size);
            strncpy(category.data(), cat.c_str(), cat_size);
            file.write(cat.c_str(), cat_size);

            // Write the space
            const auto diff_cat = get_space_size(cat_size);
            for (uint32_t i = 0; i < diff_cat; i++) {
                file.write(&buf, 1);
            }

            // Write each entry
            for (const auto &entry : reg_template[cat]) {
                // Write the name
                const uint32_t name_size = static_cast<uint32_t>(entry.name.size());
                std::vector<char> name(name_size);
                strncpy(name.data(), entry.name.c_str(), name_size);
                file.write(name.data(), name_size);

                // Write the space
                const auto diff_name = get_space_size(name_size + 1);
                for (uint32_t i = 0; i < diff_name; i++) {
                    file.write(&buf, 1);
                }

                // Write the value
                const uint32_t value_size = entry.size;
                file.write(regmgr.system_dreg[cat][entry.name].data(), value_size);

                // Write the space
                const auto diff_value = get_space_size(value_size);
                for (uint32_t i = 0; i < (diff_value + 1); i++) {
                    file.write(&buf, 1);
                }
            }
        }

        file.close();
    }
}

static void init_system_dreg(RegMgrState &regmgr) {
    // Initialize the system.dreg with the default values from the template
    for (const auto &reg : reg_template) {
        for (const auto &entry : reg.second) {
            auto &value = regmgr.system_dreg[reg.first][entry.name];
            value.resize(entry.size);
            value.assign(entry.value.begin(), entry.value.end());
        }
    }

    save_system_dreg(regmgr);
}

static bool category_or_name_is_empty(const std::string &category, const std::string &name) {
    return category.empty() || name.empty();
}

static std::string fix_category(const std::string &category) {
    return category.back() == '/' ? category : category + '/';
}

static std::string set_default_value(RegMgrState &regmgr, const std::string &category, const std::string &name) {
    if (category_or_name_is_empty(category, name))
        return {};

    const auto &reg = std::find_if(reg_template[category].begin(), reg_template[category].end(), [&](const auto &n) {
        return n.name == name;
    });

    if (reg == reg_template[category].end()) {
        LOG_ERROR("No default value found for {}{}", category, name);
        return {};
    }

    auto &sys = regmgr.system_dreg[category][name];
    sys.clear();
    sys.resize(reg->size);
    sys.assign(reg->value.begin(), reg->value.end());

    LOG_INFO("Successfully set default value for {}{}", category, name);

    save_system_dreg(regmgr);

    return reg->value;
}

void get_bin_value(RegMgrState &regmgr, const std::string &category, const std::string &name, void *buf, uint32_t bufSize) {
    if (category_or_name_is_empty(category, name))
        return;

    std::lock_guard<std::mutex> lock(regmgr.mutex);
    const auto &sys = regmgr.system_dreg[fix_category(category)][name];

    memcpy(buf, sys.data(), bufSize);
}

void set_bin_value(RegMgrState &regmgr, const std::string &category, const std::string &name, const void *buf, const uint32_t bufSize) {
    if (category_or_name_is_empty(category, name))
        return;

    std::lock_guard<std::mutex> lock(regmgr.mutex);

    const char *data = reinterpret_cast<const char *>(buf);
    regmgr.system_dreg[fix_category(category)][name].assign(data, data + bufSize);

    save_system_dreg(regmgr);
}

int32_t get_int_value(RegMgrState &regmgr, const std::string &category, const std::string &name) {
    if (category_or_name_is_empty(category, name))
        return 0;

    std::lock_guard<std::mutex> lock(regmgr.mutex);

    const auto cat = fix_category(category);
    const auto &sys = regmgr.system_dreg[cat][name];
    auto value_str = std::string(sys.begin(), sys.end());
    if (!std::all_of(value_str.begin(), value_str.end(), [](unsigned char c) { return std::isdigit(c) || (c == '\0'); })) {
        LOG_ERROR("Invalid value for {}{}: {}, attempt using default value!", cat, name, value_str);
        value_str = set_default_value(regmgr, cat, name);
        if (value_str.empty())
            return 0;
    }

    return string_utils::stoi_def(value_str);
}

void set_int_value(RegMgrState &regmgr, const std::string &category, const std::string &name, int32_t value) {
    if (category_or_name_is_empty(category, name))
        return;

    std::lock_guard<std::mutex> lock(regmgr.mutex);

    const auto str_value = std::to_string(value);
    regmgr.system_dreg[fix_category(category)][name].assign(str_value.begin(), str_value.end());

    save_system_dreg(regmgr);
}

std::string get_str_value(RegMgrState &regmgr, const std::string &category, const std::string &name) {
    if (category_or_name_is_empty(category, name))
        return {};

    std::lock_guard<std::mutex> lock(regmgr.mutex);
    const auto &sys = regmgr.system_dreg[fix_category(category)][name];

    return std::string(sys.begin(), sys.end());
}

void set_str_value(RegMgrState &regmgr, const std::string &category, const std::string &name, const char *value, const uint32_t bufSize) {
    if (category_or_name_is_empty(category, name))
        return;

    std::lock_guard<std::mutex> lock(regmgr.mutex);
    regmgr.system_dreg[fix_category(category)][name].assign(value, value + bufSize);

    save_system_dreg(regmgr);
}

void init_regmgr(RegMgrState &regmgr, const std::wstring &pref_path) {
    // Load the registry template
    const auto reg = decryptRegistryFile(fs::path(pref_path) / "os0/kd/registry.db0");
    if (reg.empty()) {
        return;
    }

    // Initialize the registry template
    init_reg_template(regmgr, reg);

    // Initialize the system.dreg
    regmgr.system_dreg_path = pref_path + L"vd0/registry/system.dreg";
    regmgr.system_dreg.clear();
    if (!load_system_dreg(regmgr)) {
        LOG_WARN("Failed to load system.dreg, attempting to create it");
        init_system_dreg(regmgr);
    }
}

std::pair<std::string, std::string> get_category_and_name_by_id(const int id, const std::string &export_name) {
    switch (id) {
    case 0x00023FC2:
        return { "/CONFIG/ACCESSIBILITY/", "large_text" };
    case 0x00033818:
        return { "/CONFIG/NP/", "env" };
    case 0x00037502:
        return { "/CONFIG/SYSTEM/", "language" };
    case 0x000504E4:
        return { "/CONFIG/NP2/TELEPORT/", "passcode_client" };
    case 0x00068303:
        return { "/CONFIG/BROWSER/ADDIN/TRENDMICRO/", "tm_ec_ttl" };
    case 0x00088776:
        return { "/CONFIG/DATE/", "date_format" };
    case 0x000A0495:
        return { "/CONFIG/NP/", "nav_only" };
    case 0x000B6ECD:
        return { "/CONFIG/NP/", "np_ad_clock_diff" };
    case 0x000B73CD:
        return { "/CONFIG/NP/", "debug" };
    case 0x000D18E5:
        return { "/CONFIG/NP/", "np_geo_filtering" };
    case 0x00100591:
        return { "/CONFIG/DATE/", "time_zone" };
    case 0x00134C03:
        return { "/CONFIG/NET/", "pspnet_adhoc_ssid_prefix" };
    case 0x00146E23:
        return { "/CONFIG/GAME/", "show_debug_info" };
    case 0x00154A2C:
        return { "/CONFIG/GAME/", "fake_free_space" };
    case 0x00156489:
        return { "/CONFIG/NP/", "debug_ingame_commerce2" };
    case 0x00168B9B:
        return { "/CONFIG/NP2/", "tpps_proxy_password" };
    case 0x00186122:
        return { "/CONFIG/SECURITY/PARENTAL/", "passcode" };
    case 0x001B2292:
        return { "/CONFIG/BROWSER/ADDIN/TRENDMICRO/", "tm_ec_ttl_update_time" };
    case 0x00229142:
        return { "/CONFIG/SYSTEM/", "button_assign" };
    case 0x0022B191:
        return { "/CONFIG/NP2/", "tpps_proxy_port" };
    case 0x0025CE9A:
        return { "/CONFIG/GAME/", "fake_free_space_quota" };
    case 0x002FDFB4:
        return { "/CONFIG/DISPLAY/", "hdmi_out_scaling_ratio" };
    case 0x00313905:
        return { "/CONFIG/NP2/", "test_patch" };
    case 0x003317A1:
        return { "/CONFIG/NP2/", "trophy_setup_dialog_debug" };
    case 0x0036F14E:
        return { "/CONFIG/NP2/TELEPORT/", "enable_media_transfer" };
    case 0x003CB6A4:
        return { "/DEVENV/TOOL/", "gpi_switch" };
    case 0x00424500:
        return { "/CONFIG/GAME/", "fake_sdslot_broken" };
    case 0x00450F32:
        return { "/CONFIG/NP/", "account_id" };
    case 0x004E7A16:
        return { "/CONFIG/NP2/TELEPORT/", "target_name" };
    case 0x004F7E60:
        return { "/CONFIG/PS4LINK/", "counter" };
    case 0x00505BCE:
        return { "/CONFIG/NP2/", "fake_ratelimit" };
    case 0x0051F6AE:
        return { "/CONFIG/SPECIFIC/", "idu_mode" };
    case 0x00528C0D:
        return { "/CONFIG/NP2/", "ignore_titleid" };
    case 0x00563BFE:
        return { "/CONFIG/NET/", "ssl_cert_ignorable" };
    case 0x00598438:
        return { "/CONFIG/SYSTEM/", "username" };
    case 0x005F6737:
        return { "/CONFIG/NP2/TWITTER/", "access_token" };
    case 0x00611DC9:
        return { "/CONFIG/ACCESSIBILITY/", "bold_text" };
    case 0x00612B3E:
        return { "/CONFIG/BROWSER/", "web_security_status" };
    case 0x00646A8E:
        return { "/CONFIG/NP2/", "tpps_proxy_server" };
    case 0x00668503:
        return { "/CONFIG/DATE/", "time_format" };
    case 0x00683DCD:
        return { "/CONFIG/SYSTEM/", "key_pad" };
    case 0x006FF829:
        return { "/CONFIG/NP2/", "tpps_proxy_flag" };
    case 0x00711659:
        return { "/CONFIG/SECURITY/PARENTAL/", "content_start_control" };
    case 0x00760538:
        return { "/CONFIG/DATE/", "summer_time" };
    case 0x007C9764:
        return { "/CONFIG/NP2/", "fake_plus" };
    case 0x007D12C4:
        return { "/CONFIG/GAME/", "fake_contents_max" };
    case 0x007F9315:
        return { "/CONFIG/DATE/", "is_summer_time" };
    case 0x0081649F:
        return { "/CONFIG/BROWSER/ADDIN/TRENDMICRO/", "tm_service" };
    case 0x00872621:
        return { "/CONFIG/BROWSER/ADDIN/TRENDMICRO/", "tm_service_sub_status" };
    case 0x0089C9CF:
        return { "/CONFIG/SECURITY/PARENTAL/", "store_start_control" };
    case 0x008A2AD7:
        return { "/CONFIG/ACCESSIBILITY/", "contrast" };
    case 0x008C3860:
        return { "/CONFIG/BROWSER/DEBUG/", "net_dbg_config" };
    case 0x008D89EB:
        return { "/CONFIG/NP2/TELEPORT/", "wol_target_mac_address" };
    case 0x008E3939:
        return { "/CONFIG/MUSIC/MUSIC_APP/", "impose_audio_balance" };
    case 0x008EB468:
        return { "/CONFIG/NP2/", "tpps_proxy_user_name" };
    case 0x008F94F9:
        return { "/CONFIG/NP/", "country" };
    case 0x0091F34F:
        return { "/CONFIG/NP2/TWITTER/", "access_token_secret" };
    case 0x0093C981:
        return { "/CONFIG/PSM/", "revocation_check_req" };
    case 0x0094E320:
        return { "/CONFIG/PS4LINK/", "keys" };
    case 0x009623D0:
        return { "/CONFIG/GAME/", "fake_no_memory_card" };
    case 0x00971FA1:
        return { "/CONFIG/SHELL/", "voice_priority" };
    case 0x00987180:
        return { "/CONFIG/NP2/TELEPORT/", "initial_target" };
    case 0x00988B81:
        return { "/CONFIG/PSNOW/", "app_cached_url" };
    default:
        LOG_WARN("{}: unknown id: {}", export_name, log_hex(id));
        return { "", "" };
    }
}

} // namespace regmgr