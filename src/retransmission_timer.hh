#include <cstdint>

class RetransmissionTimer {
public:
    RetransmissionTimer() {}
    void start(uint64_t RTO_) { RTO = RTO_; time_passed = 0; running = true; }
    void stop() {RTO = 0; time_passed = 0; running = false; }
    bool accumulate(uint64_t time) { 
        if(!running) return false;
        return (time_passed += time) >= RTO; 
    }
private:
    uint64_t RTO {};
    uint64_t time_passed {};
    bool running {};
};