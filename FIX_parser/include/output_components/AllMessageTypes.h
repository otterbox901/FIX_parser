//
// Created by alex on 2026-08-28.
//

#ifndef UNTITLED_MESSAGES_H
#define UNTITLED_MESSAGES_H
#include <string>
#include <unordered_map>

inline std::unordered_map<std::string, std::string> fix44MsgTypes = {
    /// application messages
    ///
    {"6", "Indication of Interest"}, {"7", "Advertisement"}, {"8", "Execution Report"}, {"9", "Order Cancel Reject"},
    {"AA", "Derivative Security List"}, {"AB", "New Order - Multileg"}, {"AC", "Multileg Order Cancel/Replace Request"},
    {"AD", "Trade Capture Report Request"}, {"AE", "Trade Capture Report"}, {"AF", "Order Mass Status Request"},
    {"AG", "Quote Request Reject"}, {"AH", "RFQ Request"}, {"AI", "Quote Status Report"}, {"AJ", "Quote Response"},
    {"AK", "Confirmation"}, {"AL", "Position Maintenance Request"}, {"AM", "Position Maintenance Report"},
    {"AN", "Request for Positions"}, {"AO", "Request for Positions Ack"}, {"AP", "Position Report"},
    {"AQ", "Trade Capture Report Request Ack"}, {"AR", "Trade Capture Report Ack"}, {"AS", "Allocation Report"},
    {"AT", "Allocation Report Ack"}, {"AU", "Confirmation Ack"}, {"AV", "Settlement Instruction Request"},
    {"AW", "Assignment Report"}, {"AX", "Collateral Request"}, {"AY", "Collateral Assignment"}, {"AZ", "Collateral Response"},
    {"B", "News"}, {"BA", "Collateral Report"}, {"BB", "Collateral Inquiry"}, {"BC", "Network (Counterparty System) Status Request"},
    {"BD", "Network (Counterparty System) Status Response"}, {"BE", "User Request"}, {"BF", "User Response"},
    {"BG", "Collateral Inquiry Ack"}, {"BH", "Confirmation Request"}, {"C", "Email"}, {"D", "New Order - Single"},
    {"E", "New Order - List"}, {"F", "Order Cancel Request"}, {"G", "Order Cancel/Replace Request"},
    {"H", "Order Status Request"}, {"J", "Allocation Instruction"}, {"K", "List Cancel Request"}, {"L", "List Execute"},
    {"M", "List Status Request"}, {"N", "List Status"}, {"P", "Allocation Instruction Ack"}, {"Q", "Don't Know Trade"},
    {"R", "Quote Request"}, {"S", "Quote"}, {"T", "Settlement Instructions"}, {"V", "Market Data Request"},
    {"W", "Market Data - Snapshot/Full Refresh"}, {"X", "Market Data - Incremental Refresh"},
    {"Y", "Market Data Request Reject"}, {"Z", "Quote Cancel"}, {"a", "Quote Status Request"},
    {"b", "Mass Quote Acknowledgement"}, {"c", "Security Definition Request"}, {"d", "Security Definition"},
    {"e", "Security Status Request"}, {"f", "Security Status"}, {"g", "Trading Session Status Request"},
    {"h", "Trading Session Status"}, {"i", "Mass Quote"}, {"j", "Business Message Reject"}, {"k", "Bid Request"},
    {"l", "Bid Response"}, {"m", "List Strike Price"}, {"n", "XML message"}, {"o", "Registration Instructions"},
    {"p", "Registration Instructions Response"}, {"q", "Order Mass Cancel Request"}, {"r", "Order Mass Cancel Report"},
    {"s", "New Order - Cross"}, {"t", "Cross Order Cancel/Replace Request"}, {"u", "Cross Order Cancel Request"},
    {"v", "Security Type Request"}, {"w", "Security Types"}, {"x", "Security List Request"}, {"y", "Security List"},
    {"z", "Derivative Security List Request"},

    /// session messages are added here to make it easier
    ///
    {"0", "Heartbeat"}, {"1", "Test Request"}, {"2", "Resend Request"}, {"3", "Reject"},
    {"4", "Sequence Reset"}, {"5", "Logout"}, {"A", "Logon"}
};

#endif //UNTITLED_MESSAGES_H