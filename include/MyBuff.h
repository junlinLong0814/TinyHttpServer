#ifndef MY_BUFF_H
#define MY_BUFF_H

#include <algorithm>
#include <deque>
#include <memory>
#include <numeric>
#include <string>
#include <string.h>
#include "MyLog.h"

class nonCopyable{
public:
    nonCopyable(const nonCopyable&) = delete;
    void operator = (const nonCopyable&) = delete;
protected:
    nonCopyable() = default;
    ~nonCopyable() = default;
};

class Buffer : public nonCopyable{
private:
    size_t _size {0};           //当前缓冲区拥有的数据量
    size_t _capacity {0};       //当前缓冲区可以拥有的最大数据量
    std::unique_ptr<char []> _buffer{nullptr};
public:
    Buffer() = default;

    Buffer(size_t size,size_t cap):
        _size(size),
        _capacity(cap),
        _buffer(_capacity > 0 ? new char[_capacity] : nullptr)
    {
    }

    Buffer(const char* pdata,size_t size, size_t cap):Buffer(size,cap){
        memcpy(_buffer.get(),pdata,size);
    }

    Buffer(const char* pdata,size_t size) : Buffer(pdata,size,size) {}

    Buffer(Buffer&& buf):
        _size(buf._size),
        _capacity(buf._capacity),
        _buffer(std::move(buf._buffer))
    {
        buf._size = 0;
        buf._capacity = 0;
    }

    Buffer& operator=(Buffer&& buf){
        _size = buf._size;
        _capacity = buf._capacity;
        _buffer = move(buf._buffer);

        if(buf._buffer != nullptr){
            buf._buffer.reset();
        }
        buf._capacity = 0;
        buf._size = 0;
        return *this;
    }

    ~Buffer(){
        if(_buffer != nullptr){
            _buffer.reset();
        }
    }

    bool empty() const {return _size == 0;}

    char *raw_ptr() {return _buffer.get();}

    size_t size() const {return _size;}

    size_t capacity() const { return _capacity;}

    size_t writeable_size() { return _capacity - _size;}

    size_t readable_size() {return _size;}

    void add_size(const int size) {_size += size;}
    /*append data at the buffer[size]*/
    void append_data(char* data,size_t size);

    /*copy size from _buffer to dst,then remove the last data to the begin*/
    bool read_data(char* dst, size_t size);

    /*find the str in the buffer,if be found ,call read_data()*/
    bool find_str(char* dst, char* str);

    /*read all the data*/
    size_t read_all(char *dst);

    size_t read_all(std::string& dst);

    bool read_data(std::string& dst,size_t size);

    bool find_str(std::string& dst, char* str);

    void drstory_buff(){_buffer.reset();_size = _capacity = 0;}

};


#endif