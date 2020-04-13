/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#pragma once
#include <map>

struct InterfaceElement {
    public:
        InterfaceElement(const String &p_id, const String &p_element, const String &p_content, const String &p_parent) {
            element = std::move(p_element);
            id = std::move(p_id);
            content = std::move(p_content);
            parent = std::move(p_parent);
        };

        struct cmp_str
        {
            bool operator()(const String &a, const String &b) const
            {
                return strcmp(a.c_str(), b.c_str()) < 0;
            }
        };

        const String& getId() const
        {
            return id;
        }

        String element;
        String id;
        String content;
        String parent;
        std::map<String, String, cmp_str> attributes;

        void setAttribute(const String &key, const String &value) {
            attributes[key] = value;
        };

        String getAttribute(const String &key) const {
            auto found = attributes.find(key);
            if (found != attributes.end()) {
                return found->second;
            }
            return {""};
        }
};
