#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    _capacity = capacity;
}

// capacity - byte_stream.size() = remaining_capacity
size_t ByteStream::write(const string &data) {
    size_t written = 0;
    if (data.size() <= remaining_capacity()) {
        written = data.size();
        byte_stream += data;
    } else {
        written = remaining_capacity();
        byte_stream += data.substr(0, written);
    }
    bytes_written_count += written;
    return written;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string res;
    if (byte_stream.size() <= len) {
        res = byte_stream;
    } else {
        res = byte_stream.substr(0, len); 
    }
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    if (byte_stream.size() <= len) {
        bytes_read_count += byte_stream.size();
        byte_stream = "";
    } else {
        bytes_read_count += len;
        byte_stream = byte_stream.substr(len, byte_stream.size() - len);
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { input_end = true; }
// `true` if the stream input has ended
bool ByteStream::input_ended() const { return input_end; }

size_t ByteStream::buffer_size() const { return byte_stream.size(); }

bool ByteStream::buffer_empty() const { return byte_stream.empty(); }
// `true` if the output has reached the ending
// 当输入终止并且缓冲区全部被读出，则认为读到了eof
bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return bytes_written_count; }

size_t ByteStream::bytes_read() const { return bytes_read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - byte_stream.size(); }