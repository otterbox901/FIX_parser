//
// Created by alex on 2026-08-28.
//

#ifndef UNTITLED_DECODER_H
#define UNTITLED_DECODER_H
#include <string>
#include <unordered_map>
#include <vector>

inline std::string decoder(const std::vector<char>& buffer, const ssize_t bytes_read,
                           const std::unordered_map<std::string, std::string>& tags,
                           const std::unordered_map<std::string, std::string>& message,
                           std::string& incomplete_message)
{

    incomplete_message.append(buffer.begin(), buffer.begin() + bytes_read);

    std::string key;
    std::string value;
    bool add_to = true;
    std::string leftover_state;

    for (const char c : incomplete_message) {
        if (c == 0x01) {
            if (key == "35") {
                if (message.contains(value)) {
                    //std::cout << message.at(value) << " message \n";
                } else {
                    //std::cout << "Unknown Message Type (" << value << ") \n";
                }
            }
            else {
                if (key == "8") //std::cout << "_________MESSAGE BEGIN________" << "\n";

                if (tags.contains(key)) {
                    //std::cout << tags.at(key) << ": " << value << '\n';
                } else {
                    //std::cout << "Unknown Tag (" << key << "): " << value << '\n';
                }

                if (key == "10");//std::cout << "_________MESSAGE END________" << "\n\n";
            }
            key.clear();
            value.clear();
            add_to = true;
        }
        else if (c == 0x3D) {
            add_to = false;
        }
        else {
            if (add_to) key.push_back(c);
            else value.push_back(c);
        }
    }

    if (!key.empty() || !value.empty()) {
        leftover_state = key;
        if (!add_to) leftover_state += "=" + value;
    }

    return leftover_state;
}

/* inline std::string decoder(std::vector<char>& buffer, std::unordered_map<std::string, std::string> tags,
                    std::unordered_map<std::string, std::string> message, std::string& incomplete_message)
{
    std::string key;
    std::string value;
    bool add_to= true;

    for (char& c : incomplete_message) {
        if (c== 0x01) {
            if (key== "35") {
                //std::cout<<message[value]<<" "<<"message \n";
                key.clear();
                value.clear();
                add_to= true;
            }
            else {
                if (key== "8") //std::cout<< "_________MESSAGE BEGIN________"<<"\n";
                //std::cout<<tags[key]<<" "<<value<<'\n';
                if (key == "10") //std::cout<< "_________MESSAGE END________"<<"\n\n";
                key.clear();
                value.clear();
                add_to= true;
            }
        }
        else if (c== 0x3D) add_to= false;
        else {
            if (add_to) {
                key.push_back(c);
            }
            else value.push_back(c);
        }
    }

    for (char& c : buffer) {
        if (c== 0x01) {
            if (key== "35") {
                //std::cout<<message[value]<<" message \n";
                key.clear();
                value.clear();
                add_to= true;
            }
            else {
                if (key== "8") //std::cout<< "_________MESSAGE BEGIN________"<<"\n";
                //std::cout<<tags[key]<<" "<<value<<'\n';
                if (key == "10") //std::cout<< "_________MESSAGE END________"<<"\n\n";
                key.clear();
                value.clear();
                add_to= true;
            }
        }
        else if (c== 0x3D) add_to= false;
        else {
            if (add_to) {
                key.push_back(c);
            }
            else value.push_back(c);
        }
    }
    if (add_to) return key;
    return key+"="+value;
}
*/

#endif //UNTITLED_DECODER_H