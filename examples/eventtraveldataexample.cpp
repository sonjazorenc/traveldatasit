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
//my
float SPEED,GAS_MILEAGE,MOTOR_REVOLUTIONS = 0.0;
int GEAR = 0;
//finished

class traveldata {
public:
    traveldata(bool _tcp_is_used, uint32_t _cycle) :
            ride_(vsomeip::runtime::get()->create_application()),
            is_registered_(false),
            use_tcp_(_tcp_is_used),
            cycle_(_cycle),
            blocked_(false),
            running_(true),
            is_offered_(false),
            offer_thread_(std::bind(&traveldata::run, this)),
            notify_thread_(std::bind(&traveldata::notify, this)) {
    }

    /*
    *initialization of the car
    *a new event is made
    *travel data appearing on the console
    */
    
    bool init() {
        std::lock_guard<std::mutex> its_lock(mutex_);

        if (!ride_->init()) {
            std::cerr << "Engine is not started yet." 
                      << std::endl;
            return false;
    }

        
        ride_->register_state_handler(
                std::bind(&traveldata::on_state, this,
                        std::placeholders::_1));

        ride_->register_message_handler(
                SERVICE_ID,
                INSTANCE_ID,
                GET_METHOD_ID,
                std::bind(&traveldata::on_get, this,
                          std::placeholders::_1));

        ride_->register_message_handler(
                SERVICE_ID,
                INSTANCE_ID,
                SET_METHOD_ID,
                std::bind(&traveldata::on_set, this,
                          std::placeholders::_1));

        std::set<vsomeip::eventgroup_t> its_groups;
        its_groups.insert(EVENTGROUP_ID);
        ride_->offer_event(
                SERVICE_ID,
                INSTANCE_ID,
                EVENT_ID,
                its_groups,
                true);
        current_travel_data_ = "Speed is " + SPEED + "/n" + 
                   "Gas mileage is " + GAS_MILEAGE + "/n" + 
                   "Motor revolutions are " + MOTOR_REVOLUTIONS + "/n" +
                   "Gear is " + GEAR + std::endl;
        std::cout << current_travel_data
                  << std::endl;
        blocked_ = true;
        condition_.notify_one();
        return true;
    }

    void start() {
        std::cout << "Hello, nice to see you. Engine is starting."
                  << "/n"
                  << "Gas-Mileage is " << GAS_MILEAGE
                  << "Gear is" << GEAR
                  << "Motor revolutions are" << MOTOR_REVOLUTIONS
                  << "Goodbye"
                  << std::endl;
        ride_->start();
    }

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
        
    /*
     * No event is sent anymore
     * Engine is cooling down
     */
    void stop() {
        std::cout << "Engine is cooling down."
                  << "/n"
                  << "Gas-Mileage is" << GAS_MILEAGE
                  << "Gear is" << GEAR
                  << "Motor revolutions are" << MOTOR_REVOLUTIONS
                  << "Goodbye"
                  << std::endl;
        running_ = false;
        blocked_ = true;
        condition_.notify_one();
        notify_condition_.notify_one();
        ride_->clear_all_handler();
        stop_offer();
        offer_thread_.join();
        notify_thread_.join();
        ride_->stop();
    }
#endif

    /*
    *sending an event is offered
    *travel data is offered by the car
    */
    
    void offer() {
        std::lock_guard<std::mutex> its_lock(notify_mutex_);
        ride_->offer_service(SERVICE_ID, INSTANCE_ID);
        is_offered_ = true;
        notify_condition_.notify_one();
    }

    void stop_offer() {
        ride_->stop_offer_service(SERVICE_ID, INSTANCE_ID);
        is_offered_ = false;
    }

    void on_state(vsomeip::state_type_e _state) {
        std::cout << "Application " << ride_->get_name() << " is "
        << (_state == vsomeip::state_type_e::ST_REGISTERED ?
                "registered." : "deregistered.") << std::endl;

        if (_state == vsomeip::state_type_e::ST_REGISTERED) {
            if (!is_registered_) {
                is_registered_ = true;
            }
        } else {
            is_registered_ = false;
        }
    }

    void on_get(const std::shared_ptr<vsomeip::message> &_message) {
        std::shared_ptr<vsomeip::message> its_response
            = vsomeip::runtime::get()->create_response(_message);
        its_response->set_payload(payload_);
        ride_->send(its_response, true);
    }

    void on_set(const std::shared_ptr<vsomeip::message> &_message) {
        payload_ = _message->get_payload();

        std::shared_ptr<vsomeip::message> its_response
            = vsomeip::runtime::get()->create_response(_message);
        its_response->set_payload(payload_);
        ride_->send(its_response, true);
        ride_->notify(SERVICE_ID, INSTANCE_ID,
                     EVENT_ID, payload_);
    }

    void run() {
        std::unique_lock<std::mutex> its_lock(mutex_);
        while (!blocked_)
            condition_.wait(its_lock);

        bool is_offer(true);
        while (running_) {
            if (is_offer)
                offer();
            else
                stop_offer();

            for (int i = 0; i < 10 && running_; i++)
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            is_offer = !is_offer;
        }
    }

    void notify() {
        std::shared_ptr<vsomeip::message> its_message
            = vsomeip::runtime::get()->create_request(use_tcp_);

        its_message->set_service(SERVICE_ID);
        its_message->set_instance(INSTANCE_ID);
        its_message->set_method(SET_METHOD_ID);

        vsomeip::byte_t its_data[10];
        uint32_t its_size = 1;

        while (running_) {
            std::unique_lock<std::mutex> its_lock(notify_mutex_);
            while (!is_offered_ && running_)
                notify_condition_.wait(its_lock);
            while (is_offered_ && running_) {
                if (its_size == sizeof(its_data))
                    its_size = 1;

                for (uint32_t i = 0; i < its_size; ++i)
                    its_data[i] = static_cast<uint8_t>(i);

                payload_->set_data(its_data, its_size);

                std::cout << "Setting event (Length=" << std::dec << its_size << ")." << std::endl;
                app_->notify(SERVICE_ID, INSTANCE_ID, EVENT_ID, payload_);

                its_size++;

                std::this_thread::sleep_for(std::chrono::milliseconds(cycle_));
            }
        }
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    bool is_registered_;
    bool use_tcp_;
    uint32_t cycle_;

    std::mutex mutex_;
    std::condition_variable condition_;
    bool blocked_;
    bool running_;

    std::mutex notify_mutex_;
    std::condition_variable notify_condition_;
    bool is_offered_;

    std::shared_ptr<vsomeip::payload> payload_;

    // blocked_ / is_offered_ must be initialized before starting the threads!
    std::thread offer_thread_;
    std::thread notify_thread_;
};

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    traveldata *its_sample_ptr(nullptr);
    void handle_signal(int _signal) {
        if (its_sample_ptr != nullptr &&
                (_signal == SIGINT || _signal == SIGTERM))
            its_sample_ptr->stop();
    }
#endif

int main(int argc, char **pointerofpointer) {
    bool use_tcp = false;
    uint32_t cycle = 1000; // default 1s

    std::string tcp_enable("--tcp");
    std::string udp_enable("--udp");
    std::string cycle_arg("--cycle");

    for (int x = 1; x < argc; x++) {
        if (tcp_enable == pointerofpointer[x]) {
            use_tcp = true;
            break;
        }
        if (udp_enable == pointerofpointer[x]) {
            use_tcp = false;
            break;
        }

        if (cycle_arg == pointerofpointer[x] && x + 1 < pointerofpointer) {
            x++;
            std::stringstream converter;
            converter << pointerofpointer[x];
            converter >> cycle;
        }
    }

    traveldata its_sample(use_tcp_, cycle);
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
