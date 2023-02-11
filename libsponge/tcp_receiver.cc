#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn) {
        // 若连接已建立
        if (_syn) return;
        // 若连接未建立
        _syn = true;
        isn = seg.header().seqno;
        _ackno = 1;
    }
    if (!_syn) return;
    bool eof = false;
    if (seg.header().fin) {
        _fin = true;
        eof = true;
    }
    size_t index = 0;
    // if (seg.header().seqno != isn) {
    if (!seg.header().syn) {
        index = unwrap(seg.header().seqno - 1, isn, _reassembler.stream_out().buffer_size());
    } else {
        index = unwrap(seg.header().seqno, isn, _reassembler.stream_out().buffer_size());
    }
    std::string payload = seg.payload().copy();
    size_t stream_size = _reassembler.stream_out().buffer_size();
    _reassembler.push_substring(payload, index, eof);
    _ackno += _reassembler.stream_out().buffer_size() - stream_size;
    // _ackno = _reassembler.stream_out().buffer_size() + 1;
    // std::cout << _reassembler.stream_out().buffer_size() << std::endl;
    if (_fin && _reassembler.unassembled_bytes() == 0) 
        _ackno++;
}
// 只要记录isn，seqno + bytestream.size = ackno
optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!_syn) {
        return {};
    } else {
        return wrap(_ackno, isn); 
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
