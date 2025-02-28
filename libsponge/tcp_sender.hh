#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

class Timer {
private:
  size_t _rto = 0;
  size_t elapsed_time = 0;
  bool running = false;

public:
  Timer() {}
  void enable(size_t rto) { if (!running) { running = true; _rto = rto; } }
  void disable() { running = false; elapsed_time = 0; }

  void tick(const size_t ms_since_last_tick) { elapsed_time += ms_since_last_tick; }

  bool is_timeout() { return elapsed_time >= _rto; }

  void reset(size_t new_rto = 0) { running = true; _rto = new_rto; elapsed_time = 0; }
};

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};
    std::queue<TCPSegment> _segments_unacknowledged{};

    //! retransmission timer for the connection
    size_t _initial_retransmission_timeout;
    size_t _retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    Timer _timer{};

    size_t _window_size = 1;
    size_t _bytes_in_flight = 0;
    int _consecutive_retransmissions = 0;
    bool _fin = false;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    bool ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create segment for specified fill_window
    bool send_segment(TCPSegment &segment, const size_t bytes_to_send);

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}

    size_t remaining_outbound_capacity() const;

    bool is_fin() const { return _fin; }
};

// 每个TCPSender对应不止一个ticker，每次调用tick，遍历set，调用elapsed，判断是否超时，如果超时则重发segment，

// sender周期性调用tick方法判断有没有超时
// 如果超时没有收到确认号，则应该重发报文
// 1. 每隔几毫秒调用tick，参数代表距离上次调用的时间，也就代表了TCPSender保持活跃的时间
// 2. 构造TCPSender时，指定参数RTO的初始值——代表距离下次重发的时间， 
//        _initial_retransmission_timeout中保存初始值。RTO可变，但是初始值不变
// 3. timer：RTO超时后alarm，对时间的计数来自于tick
// 4. 每次长度非0的segment发送后，如果timer没有运行，则开始运行timer，
// 5. 接受到对应的响应后，停止timer
// 6. 如果调用了tick，并且retransmission timer超时，
//          a) 重发最早的没有被完全确认的segment，
//          b) 如果window size不为0
//                跟踪the number of consecutive retrasmissions(指的是重发的报文数？)
//                RTO加倍--快速退避，减少重传
//          c) RTO超时后重置timer，
// 7. 接收到新的更大的ackno时
//             将RTO设置回初始值
//             如果有outstanding data, 重置timer
//             consecutive retrasmissions 设置为0;
#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH