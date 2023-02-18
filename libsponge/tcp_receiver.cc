#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    std::cout << "received: seqno: " <<  seg.header().seqno << "SYN: " << seg.header().syn 
        << "ackno: " << seg.header().ackno << "ack: " << seg.header().ack << "fin: " << seg.header().fin 
        << "length_in_seq_space: " << seg.length_in_sequence_space() << std::endl;
    if (ackno().has_value())
        std::cout << "current ackno: " << ackno().value() << std::endl;
    // bool flag = ackno().has_value();
    if (seg.header().syn) {
        // 若连接已建立
        if (_syn) return;
        // 若连接未建立
        _syn = true;
        isn = seg.header().seqno;
        _ackno = 1;
    } else if (!_syn || 
        unwrap(seg.header().seqno, isn, _reassembler.stream_out().buffer_size()) + seg.length_in_sequence_space() < _ackno) 
            return;
    // if (seg.length_in_sequence_space() == 0) {
    //     _ackno++;
    // }
    bool eof = false;
    if (seg.header().fin) {
        _fin = true;
        eof = true;
    }
    size_t index = 0;
    std::cout << "seqno: " << seg.header().seqno << "isn: " << isn << "buffer_size: " << _reassembler.stream_out().buffer_size() << std::endl;
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
    std::cout << "new ackno: " << _ackno << std::endl;
    // _ackno = _reassembler.stream_out().buffer_size() + 1;
    // std::cout << _reassembler.stream_out().buffer_size() << std::endl;
    if (_fin && _reassembler.unassembled_bytes() == 0) 
        _ackno++;
    // std::cout << "keep alive, update _ackno" << std::endl;
    // if (seg.header().seqno == ackno().value()) {
    //     _ackno = unwrap(seg.header().seqno, isn, _reassembler.stream_out().buffer_size()) + 1;
    // }
    // if (flag && seg.length_in_sequence_space() == 0) {
    //      _ackno = unwrap(seg.header().seqno, isn, _reassembler.stream_out().buffer_size()) + 1;
    // }
    // std::cout << "syn: " << seg.header().syn << "fin: " << seg.header().fin << "payload: " << seg.payload().size() << "seqno: " << seg.header().seqno << std::endl;
    // std::cout << "absolute ackno: " << _ackno << std::endl; 
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
