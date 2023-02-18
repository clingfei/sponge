#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    std::cout << "push_string: " << data << index << eof << std::endl;
    size_t right = index + data.size();
    size_t bound = next_index + _capacity - _output.buffer_size();
    if (right <= next_index) {
        if (eof) _eof = true;
        if (_eof && empty()) _output.end_input();
        return;
    }
    if (index >= bound) return;
    bool end = eof;
    std::string substring = std::move(data);
    if (right > bound) {
        substring = substring.substr(0, substring.size() - (right - bound));
        end = false;
    }
    if (index <= next_index) {
        substring = substring.substr(next_index - index, substring.size() - (next_index - index));
        // std::cout << "substring: " << substring << std::endl;
        next_index += substring.size();
        _output.write(substring);
        if (end) _eof = true;
        
        while (!unorder_bytes.empty()) {
            auto iter = unorder_bytes.begin();
            if ((*iter).index <= next_index) {
                auto str = (*iter).data;
                bool flag = (*iter).eof;
                size_t idx = (*iter).index;
                unorder_bytes.erase(*iter);
                unassembled_count -= str.size();
                push_substring(str, idx, flag);
                continue;
            }
            break;
        }
        // if (!unorder_bytes.empty()) {
        //     auto iter = unorder_bytes.begin();
        //     if ((*iter).index <= next_index) {
        //         auto str = (*iter).data;
        //         bool flag = (*iter).eof;
        //         size_t idx = (*iter).index;
        //         unorder_bytes.erase(*iter);
        //         unassembled_count -= str.size();
        //         push_substring(str, idx, flag);
        //     }  
        // }
        // 判断set中是否有可插入的内容
    } else {
        merge_block(block(substring, end, index));
    }
    if (_eof && empty()) _output.end_input();
}

void StreamReassembler::merge_block(const block &b) {
    for (auto iter = unorder_bytes.begin(); iter != unorder_bytes.end(); iter++) {
        if (b.index + b.data.size() < (*iter).index) {
            unorder_bytes.insert(b);
            unassembled_count += b.data.size();
            return;
        } else if (b.index < (*iter).index && b.index + b.data.size() >= (*iter).index 
                        && b.index + b.data.size() < (*iter).index + (*iter).data.size()) {
            size_t index = b.index;
            std::string str = b.data.substr(0, (*iter).index - b.index) + (*iter).data;
            bool eof = (*iter).eof || b.eof;
            unassembled_count -= (*iter).data.size();
            unassembled_count += str.size();
            unorder_bytes.erase(*iter);
            unorder_bytes.insert(block(str, eof, index));
            return;
        } else if (b.index >= (*iter).index && b.index + b.data.size() <= (*iter).index + (*iter).data.size()) {
            return;
        } else if (b.index <= (*iter).index && b.index + b.data.size() >= (*iter).index + (*iter).data.size()) {
            unassembled_count -= (*iter).data.size();
            // unassembled_count += b.data.size();
            unorder_bytes.erase(*iter);
            // unorder_bytes.insert(b);
            merge_block(b);
            return;
        } else if (b.index >= (*iter).index && b.index <= (*iter).index + (*iter).data.size() 
                        && b.index + b.data.size() > (*iter).index + (*iter).data.size()) {
            size_t index = (*iter).index;
            std::string str = (*iter).data.substr(0, b.index - (*iter).index) + b.data;
            bool eof = (*iter).eof || b.eof;
            unassembled_count -= (*iter).data.size();
            // unassembled_count += str.size();
            unorder_bytes.erase(*iter);
            //unorder_bytes.insert(block(str, eof, index));
            merge_block(block(str, eof, index));
            return;
        }
    }
    unorder_bytes.insert(b);
    unassembled_count += b.data.size();
}

void StreamReassembler::clear() {
    std::set<block> s;
    swap(s, unorder_bytes);
}

size_t StreamReassembler::unassembled_bytes() const { return unassembled_count; }

bool StreamReassembler::empty() const { return unassembled_count == 0; }
