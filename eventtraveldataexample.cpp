// Example of a event sent by the server to the client 
// Transported information is the current travel data

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
#include <csignal>
#endif
#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <vsomeip/vsomeip.hpp>

#include "traveldataids.hpp"

class service_sample {
public:
    service_sample(bool _use_tcp, uint32_t _cycle) :
            app_(vsomeip::runtime::get()->create_application()),
            is_registered_(false),
            use_tcp_(_use_tcp),
            cycle_(_cycle),
            blocked_(false),
            running_(true),
            is_offered_(false),
            offer_thread_(std::bind(&service_sample::run, this)),
            notify_thread_(std::bind(&service_sample::notify, this)) {
    }

    bool init() {
        std::lock_guard<std::mutex> its_lock(mutex_);

        if (!app_->init()) {
            std::cerr << "No event-sending possible because no apüplication already initialized" << std::endl;
            return false;
}


