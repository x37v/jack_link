// jack_link.cpp
//

#include "jack_link.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <cctype>


jack_link::jack_link (void) : m_link(120.0), m_pJackClient(NULL)
{
	initialize();
}


jack_link::~jack_link (void)
{
	terminate();
}

void jack_link::timebase_callback (
	jack_transport_state_t state, jack_nframes_t nframes,
	jack_position_t *position, int new_pos, void *pvUserData )
{
	jack_link *pJackLink = static_cast<jack_link *> (pvUserData);
	return pJackLink->timebase_callback(state, nframes, position, new_pos);
}


void jack_link::timebase_callback (
	jack_transport_state_t state, jack_nframes_t nframes,
	jack_position_t *position, int new_pos )
{
	const auto time = std::chrono::microseconds(
		llround(1.0e6 * position->frame / position->frame_rate));

	const auto timeline = m_link.captureAppTimeline();
	const auto quantum = 4.0;//engine.quantum();

	const double beats_per_minute = timeline.tempo();
	const double beats_per_bar = std::max(quantum, 1.);

	const double beats = beats_per_minute * time.count() / 60.e6;
	const double bar = std::floor(beats / beats_per_bar);
	const double beat = beats - bar * beats_per_bar;

	static const double ticks_per_beat = 960.0;
	static const float beat_type = 4.0f;

	position->valid = JackPositionBBT;
	position->bar = int32_t(bar) + 1;
	position->beat = int32_t(beat) + 1;
	position->tick = int32_t(ticks_per_beat * beat / beats_per_bar);
	position->beats_per_bar = float(beats_per_bar);
	position->ticks_per_beat = ticks_per_beat;
	position->beats_per_minute = beats_per_minute;
	position->beat_type = beat_type;
}


void jack_link::initialize (void)
{
	jack_status_t status = JackFailure;
	m_pJackClient = ::jack_client_open("jack_link", JackNullOption, &status);
	if (m_pJackClient == NULL) {
		std::cerr << "Could not initialize JACK client:" << std::endl;
		if (status & JackFailure)
			std::cerr << "Overall operation failed." << std::endl;
		if (status & JackInvalidOption)
			std::cerr << "Invalid or unsupported option." << std::endl;
		if (status & JackNameNotUnique)
			std::cerr << "Client name not unique." << std::endl;
		if (status & JackServerStarted)
			std::cerr << "Server is started." << std::endl;
		if (status & JackServerFailed)
			std::cerr << "Unable to connect to server." << std::endl;
		if (status & JackServerError)
			std::cerr << "Server communication error." << std::endl;
		if (status & JackNoSuchClient)
			std::cerr << "Client does not exist." << std::endl;
		if (status & JackLoadFailure)
			std::cerr << "Unable to load internal client." << std::endl;
		if (status & JackInitFailure)
			std::cerr << "Unable to initialize client." << std::endl;
		if (status & JackShmFailure)
			std::cerr << "Unable to access shared memory." << std::endl;
		if (status & JackVersionError)
			std::cerr << "Client protocol version mismatch." << std::endl;
		std::cerr << std::endl;
		std::terminate();
	};

	::jack_set_timebase_callback(
		m_pJackClient, 0, jack_link::timebase_callback, this);

	::jack_activate(m_pJackClient);
}


void jack_link::terminate (void)
{
	if (m_pJackClient) {
		::jack_deactivate(m_pJackClient);
		::jack_client_close(m_pJackClient);
		m_pJackClient = NULL;
	}
}


int main ( int, char ** )
{
	jack_link app;

	std::string line;
	while (line.compare("quit")) {
		std::cout << "jack_link> ";
		getline(std::cin, line);
		std::transform(
			line.begin(), line.end(),
			line.begin(), ::tolower);
	}

	return 0;
}


// end of jack_link.cpp
