#include <fstream>
#include <string>
#include <vector>

std::vector<std::string> parse_ini(const std::string& filename, const std::string& section) {
    std::ifstream file(filename);
    std::string line;
    bool in_section = false;
    std::vector<std::string> entries;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue; // Skip comments and empty lines
        }

        if (line.front() == '[' && line.back() == ']') {
            std::string current_section = line.substr(1, line.size() - 2);
            in_section = (current_section == section);
            continue;
        }

        if (in_section) {
            size_t eq_pos = line.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = line.substr(0, eq_pos);
                std::string value = line.substr(eq_pos + 1);
                // Trim whitespace from key and value
                key.erase(0, key.find_first_not_of(" \t\n\r"));
                key.erase(key.find_last_not_of(" \t\n\r") + 1);
                value.erase(0, value.find_first_not_of(" \t\n\r"));
                value.erase(value.find_last_not_of(" \t\n\r") + 1);
                entries.push_back(key + "=" + value);
            }
        }
    }
    return entries;
}