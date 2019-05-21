//the subscription by the client which enables the server to send
//the client in specific time intervals the latest travel data


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

class client_sample {
  public:
    client_sample(bool _use_tcp) :
            app_(vsomeip::runtime::get()->create_application()), use_tcp_(
                    _use_tcp) {
    }

   bool init() {
        if (!app_->init()) {
            std::cerr << "Not possible to send travel data because no application initialized currently" << std::endl;
            return false;
        }
     
      std::cout << "Client settings [protocol="
                << (use_tcp_ ? "TCP" : "UDP")
                << "]"
                << std::endl;
     
     
  
  
