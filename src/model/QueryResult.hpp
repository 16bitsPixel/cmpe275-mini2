#pragma once

#include <cstdint>
#include <string>

struct QueryResultRow {
    uint32_t rowId = 0;
    std::string sourceNodeId;
    int32_t vendorId = 0;
    int64_t pickupDatetime = 0;
    int64_t dropoffDatetime = 0;
    int32_t passengerCount = 0;
    float tripDistance = 0.0f;
    int32_t rateCodeId = 0;
    std::string storeAndFwdFlag;
    int32_t puLocationId = 0;
    int32_t doLocationId = 0;
    int32_t paymentType = 0;
    int32_t fareAmount = 0;
    int32_t extra = 0;
    int32_t mtaTax = 0;
    int32_t tipAmount = 0;
    int32_t tollsAmount = 0;
    int32_t improvementSurcharge = 0;
    int32_t totalAmount = 0;
    int32_t congestionSurcharge = 0;
};