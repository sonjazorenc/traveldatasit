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


// Creation of a new application.
// : a new ride is started

class driver {
  public:
    driver(bool _tcp_is_used) :
            ride_(vsomeip::runtime::get()->create_application()), 
                 use_tcp_(_tcp_is_used) {
    }
  
/*
proofing if the initialization of the application has still taken place
if the engine has already started the driver setting procol appears on the console
*/

  // : Question : is the engine started yet
  //if not: false
  //if yes: further operations and true
  
   bool init() {
        if (!ride_->init()) {
            std::cerr << "Not possible to send travel data because engine has not started yet" 
                      << std::endl;
            return false;
        }
     
      std::cout << "Driver settings [protocol="
                << (_tcp_is_used ? "TCP" : "UDP")
                << "] Asking for the current travel data started"
                << std::endl;
     
      ride_->register_state_handler(
                std::bind(&driver::on_state, this,
                std::placeholders::_1));
     
      ride_->register_message_handler(
                vsomeip::ANY_SERVICE, INSTANCE_ID, vsomeip::ANY_METHOD,
                std::bind(&driver::on_message, this,
                        std::placeholders::_1));

      ride_->register_availability_handler(SERVICE_ID, INSTANCE_ID,
                std::bind(&driver::on_availability,
                          this,
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
     
   std::set<vsomeip::eventgroup_t> its_groups;
        its_groups.insert(EVENTGROUP_ID);
        ride_->request_event(
                SERVICE_ID,
                INSTANCE_ID,
                EVENT_ID,
                its_groups,
                true);
        ride_->subscribe(SERVICE_ID, INSTANCE_ID, EVENTGROUP_ID);

        return true;
    }

  
  //final start of the application
  //: engine is finally started
  
    int start() {
        ride_->start();
        std::cout << "Car is ready"
                  << std::endl;
    }

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    /*
     * application is stopped
     * engine is cooled down
     */
    
    int stop() {
        ride_->clear_all_handler();
        ride_->unsubscribe(SERVICE_ID, INSTANCE_ID, EVENTGROUP_ID);
        ride_->release_event(SERVICE_ID, INSTANCE_ID, EVENT_ID);
        ride_->release_service(SERVICE_ID, INSTANCE_ID);
        ride_->stop();
        std::cout << "Travel data not required anymore"
                  << std::endl;
        return 0;
    }
#endif

    void on_state(vsomeip::state_type_e _state) {
        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            ride_->request_service(SERVICE_ID, INSTANCE_ID);
        }
    }

    int on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available) {
        std::cout << "Travel data["
                << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                << "] is "
                << (_is_available ? "available." : "NOT available.")
                << std::endl;
        return 0;
}
  
    void on_message(const std::shared_ptr<vsomeip::message> &_response) {
               std::stringstream its_message;
        its_message << "Received a notification for Event ["
                    << "/n"
                    << " Getting Travel Data]"
                    <<std::endl;
        std::shared_ptr<vsomeip::payload> its_payload =
                _response->get_payload();
        its_message << "(" << std::dec << its_payload->get_length() << ") ";
        for (uint32_t i = 0; i < its_payload->get_length(); ++i)
            its_message << std::hex << std::setw(2) << std::setfill('0')
                << (int) its_payload->get_data()[i] << " ";
             std::cout << its_message.str() << std::endl;

        if (_response->get_client() == 0) {
            if ((its_payload->get_length() % 5) == 0) {
                std::shared_ptr<vsomeip::message> its_get
                    = vsomeip::runtime::get()->create_request();
                its_get->set_service(SERVICE_ID);
                its_get->set_instance(INSTANCE_ID);
                its_get->set_method(GET_METHOD_ID);
                its_get->set_reliable(_tcp_is_used);
                app_->send(its_get, true);
            }

            if ((its_payload->get_length() % 8) == 0) {
                std::shared_ptr<vsomeip::message> its_set
                    = vsomeip::runtime::get()->create_request();
                its_set->set_service(SERVICE_ID);
                its_set->set_instance(INSTANCE_ID);
                its_set->set_method(SET_METHOD_ID);
                its_set->set_reliable(_tcp_is_used);

                const vsomeip::byte_t its_data[]
                    = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                        0x48, 0x49, 0x50, 0x51, 0x52 };
                std::shared_ptr<vsomeip::payload> its_set_payload
                    = vsomeip::runtime::get()->create_payload();
                its_set_payload->set_data(its_data, sizeof(its_data));
                its_set->set_payload(its_set_payload);
                ride_->send(its_set, true);
            }
        }
    }

private:
    std::shared_ptr< vsomeip::application > ride_;
    bool _using_tcp;
};

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    driver *its_sample_ptr(nullptr);
    void handle_signal(int _signal) {
        if (its_sample_ptr != nullptr &&
                (_signal == SIGINT || _signal == SIGTERM))
            its_sample_ptr->stop();
    }
#endif

int main(int argc, char **argv) {
    bool _using_tcp = false;

    std::string tcp_enable("--tcp");
    std::string udp_enable("--udp");

   for(c = 1; c < argc; c++){ 
 
        if (tcp_enable == argv[c]) {
            _using_tcp = true;
        } else if (udp_enable == argv[c]) {
            _using_tcp = false;
        }
   }

    driver its_sample(_using_tcp);
#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    its_sample_ptr = &its_sample;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#endif
    if (its_sample.init()) {
        its_sample.start();
        return 0;
    } else {
        return 1;
    }
}
