#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

bool TCPSender::send_segment(TCPSegment &segment, const size_t length) {
    std::cout << "length: "<< length << std::endl;
    if (_next_seqno == 0) 
        segment.header().syn = true;
    segment.header().seqno = wrap(_next_seqno, _isn);
    if (length < size_t(segment.header().syn))
        return false;
    // MAX_PAYLOAD_SIZE limits payload only.
    std::string payload = _stream.read(min(TCPConfig::MAX_PAYLOAD_SIZE, length - segment.header().syn));
    segment.payload() = Buffer(std::move(payload));
    if (!_fin && _stream.eof() && length > segment.length_in_sequence_space()) {
        _fin = true;
        segment.header().fin = true;
    }
    std::cout << "payload: " << segment.payload().str() << std::endl;
    std::cout << segment.length_in_sequence_space() << _stream.eof() << std::endl;
    if (segment.length_in_sequence_space() == 0)
        return false;
    else {
        _next_seqno += segment.length_in_sequence_space();
        _bytes_in_flight += segment.length_in_sequence_space();
        _segments_out.push(segment);
        _segments_unacknowledged.push(segment);
        return !_stream.eof();
    }
}

// 长度非0的segment发送后，如果timer未启动，启动timer
void TCPSender::fill_window() {
    // 发送窗口 = 已发送未确认 + 可发送但未发送
    // 发送窗口由接收方的窗口大小决定
    size_t fill_window = 0;
    // if receiver announce a window size of zero, fill_window method should act like the window_size is one
    if (_window_size == 0) {
        if (_bytes_in_flight == 0) {
            TCPSegment segment;
            send_segment(segment, 1);
        } else 
            return;
    } else {
        while (true) {
            TCPSegment segment;
            if (_window_size > _bytes_in_flight) 
                fill_window = _window_size - _bytes_in_flight;
            else 
                fill_window = 0;
            std::cout << "fill_window: " << fill_window << std::endl;
            if (!send_segment(segment, fill_window))
                break;
        }
    }
    if (!_segments_unacknowledged.empty()) 
        _timer.enable(_retransmission_timeout);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t _ackno = unwrap(ackno, _isn, _next_seqno);
    _window_size = window_size;
    std::cout << "_ackno: " << _ackno << " _next_seqno: " << _next_seqno << " _bytes_in_flight: " << _bytes_in_flight << std::endl;
    if (_ackno > _next_seqno || _ackno <= _next_seqno - _bytes_in_flight) 
        return;
    bool reset = false;
    // look through its collection of outstanding segments and remove any that have now been fully acknowledged
    while (!_segments_unacknowledged.empty()) {
        TCPSegment segment = _segments_unacknowledged.front();
        std::cout << "lentgh in sequence space: " << segment.length_in_sequence_space() << std::endl;
        std::cout << "unwrap: " << unwrap(segment.header().seqno, _isn, _next_seqno) << std::endl;
        if (unwrap(segment.header().seqno, _isn, _next_seqno) + segment.length_in_sequence_space() <= _ackno) {
            _bytes_in_flight -= segment.length_in_sequence_space();
            std::cout << "bytes in flight: " << _bytes_in_flight << std::endl;
            _segments_unacknowledged.pop();
            reset = true;
        } else 
            break;
    }
    if (reset) {
        _retransmission_timeout = _initial_retransmission_timeout;
        _consecutive_retransmissions = 0;
        if (!_segments_unacknowledged.empty()) 
            _timer.reset(_retransmission_timeout);
        else 
            _timer.disable();
    }
 }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if (_segments_unacknowledged.empty()) 
        return;
    _timer.tick(ms_since_last_tick);
    if (_timer.is_timeout()) {
        _segments_out.push(_segments_unacknowledged.front());
        if (_window_size > 0) {
            _retransmission_timeout *= 2;
            _consecutive_retransmissions++;
        }
        _timer.reset(_retransmission_timeout);
    }
 }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}
