#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <tuple>
#include <algorithm>
#include <atomic>
#include <thread>
#include <string>

enum class EventType {
    EventA,
    EventB,
};

template<typename EventEnum, typename LockType = std::shared_mutex>
class EventDispatcher {
public:
    using HandlerId = size_t;

private:
    struct HandlerBase {
        HandlerId id;
        int priority;

        HandlerBase(HandlerId id_, int priority_) : id(id_), priority(priority_) {}
        virtual ~HandlerBase() = default;
        virtual void call(void* args) = 0;
    };

    template<typename... Args>
    struct Handler : HandlerBase {
        std::function<void(Args...)> func;

        Handler(HandlerId id_, int priority_, std::function<void(Args...)> func_)
            : HandlerBase(id_, priority_), func(std::move(func_)) {}

        void call(void* args) override {
            auto& tup = *reinterpret_cast<std::tuple<Args...>*>(args);
            call_impl(tup, std::index_sequence_for<Args...>{});
        }

    private:
        template<typename Tuple, std::size_t... I>
        void call_impl(Tuple& tup, std::index_sequence<I...>) {
            func(std::get<I>(tup)...);
        }
    };

    std::map<EventEnum, std::vector<std::unique_ptr<HandlerBase>>> handlers_;
    mutable LockType mutex_;
    std::atomic<HandlerId> next_id_{1};

public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;

    template<typename... Args, typename Callable>
    HandlerId registerHandler(EventEnum event, Callable&& callable, int priority = 0) {
        std::unique_lock lock(mutex_);
        HandlerId id = next_id_++;
        auto handler = std::make_unique<Handler<Args...>>(id, priority, std::forward<Callable>(callable));
        auto& vec = handlers_[event];

        auto it = std::upper_bound(vec.begin(), vec.end(), priority,
            [](int p, const std::unique_ptr<HandlerBase>& h) {
                return p > h->priority;
            });
        vec.insert(it, std::move(handler));
        return id;
    }

    void unregisterHandler(EventEnum event, HandlerId id) {
        std::unique_lock lock(mutex_);
        auto it = handlers_.find(event);
        if (it != handlers_.end()) {
            auto& vec = it->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [id](const std::unique_ptr<HandlerBase>& h) {
                    return h->id == id;
                }), vec.end());
        }
    }

    template<typename... Args>
    void dispatch(EventEnum event, Args&&... args) const {
        std::shared_lock lock(mutex_);
        auto it = handlers_.find(event);
        if (it == handlers_.end()) return;

        std::tuple<std::decay_t<Args>...> tup(std::forward<Args>(args)...);
        for (auto& handler : it->second) {
            handler->call(&tup);
        }
    }
};

void benchmark() {
    EventDispatcher<EventType> dispatcher;

    dispatcher.registerHandler<int>(EventType::EventA, [](int x) { volatile int y = x + 1; }, 5);
    dispatcher.registerHandler<int>(EventType::EventA, [](int x) { volatile int y = x * 2; }, 10);
    dispatcher.registerHandler<int>(EventType::EventA, [](int x) { volatile int y = x - 3; }, 7);

    constexpr int NUM_EVENTS = 1'000'000;
    std::cout << "[Benchmark] Dispatching " << NUM_EVENTS << " events...\n";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_EVENTS; ++i) {
        dispatcher.dispatch(EventType::EventA, i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "[Benchmark] Completed in " << elapsed.count() << " seconds.\n\n";
}

void unitTests() {
    EventDispatcher<EventType> dispatcher;

    std::cout << "[Unit Test] Registering 3 handlers...\n";

    auto id1 = dispatcher.registerHandler<std::string>(EventType::EventB,
        [](const std::string& msg) { std::cout << "Handler 1: " << msg << "\n"; }, 5);
    auto id2 = dispatcher.registerHandler<std::string>(EventType::EventB,
        [](const std::string& msg) { std::cout << "Handler 2: " << msg << "\n"; }, 10);
    auto id3 = dispatcher.registerHandler<std::string>(EventType::EventB,
        [](const std::string& msg) { std::cout << "Handler 3: " << msg << "\n"; }, 1);

    std::cout << "[Unit Test] Dispatching 'Hello World'...\n";
    dispatcher.dispatch(EventType::EventB, std::string("Hello World"));

    std::cout << "[Unit Test] Unregistering Handler 2...\n";
    dispatcher.unregisterHandler(EventType::EventB, id2);

    std::cout << "[Unit Test] Dispatching again...\n";
    dispatcher.dispatch(EventType::EventB, std::string("Second Call"));
    std::cout << "\n";
}

void multithreadedTest() {
    EventDispatcher<EventType> dispatcher;

    dispatcher.registerHandler<int>(EventType::EventA, [](int x) {
        volatile int y = x * 3;
    }, 5);

    constexpr int THREADS = 4;
    constexpr int CALLS_PER_THREAD = 250'000;

    std::cout << "[Multithreaded Test] Dispatching " << THREADS * CALLS_PER_THREAD << " events using " << THREADS << " threads...\n";

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < CALLS_PER_THREAD; ++i) {
                dispatcher.dispatch(EventType::EventA, i + t);
            }
        });
    }

    for (auto& th : threads) th.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "[Multithreaded Test] Completed in " << elapsed.count() << " seconds.\n\n";
}

int main() {
    std::cout << "== Event Dispatcher Tests ==\n\n";
    unitTests();
    benchmark();
    multithreadedTest();
    return 0;
}


