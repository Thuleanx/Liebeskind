#pragma once

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <vector>

template <typename Event, typename... Data>
class EventSystem {
   public:
	using EventListener = std::function<void(Data... data)>;

	void Register(Event event, EventListener listener);
	void Remove(Event event, EventListener listener);
	void Trigger(const Event& event, Data... data);

   private:
	std::unordered_map<Event, std::vector<EventListener>> listeners;
};

template <typename Event, typename... Data>
void EventSystem<Event, Data...>::Register(
	Event event, EventListener listener
) {
	listeners[event].push_back(listener);
}

template <typename Event, typename... Data>
void EventSystem<Event, Data...>::Remove(Event event, EventListener listener) {
    if (listeners.contains(event)) {
        listeners[event].erase(
            std::find(listeners[event].begin(), listeners[event].end(), listener)
        );
    }
}

template <typename Event, typename... Data>
void EventSystem<Event, Data...>::Trigger(const Event& event, Data... data) {
	if (!listeners.contains(event)) return;
	for (const EventListener& listener : listeners[event]) listener(data...);
}
