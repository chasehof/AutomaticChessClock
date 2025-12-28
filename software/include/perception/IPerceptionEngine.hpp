#pragma once 

#include <optional>
#include <vector>


#include "types/ChessEvents.hpp"


namespace ChessClock {

    class IPerceptionEngine {
    public:
        virtual ~IPerceptionEngine() = default;

        virtual bool initialize() = 0;
        virtual bool calibrate() = 0;

        virtual void start() = 0;
        virtual void stop() = 0;

        virtual PerceptionState state() const = 0;

    virtual std::vector<PerceptionEvent> pollPerceptionEvents() = 0;

        virtual bool isCalibrated() const = 0;


    };


} // namespace ChessClock
