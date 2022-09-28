#include "MyBuff.h"

void Buffer::append_data(char* data,size_t size){
    if(writeable_size() > size){
        /*剩余空间足够写*/
        memcpy(_buffer.get(),data,size);
        _size += size;
    }else{
        /*剩余空间不够写 扩容*/
        size_t new_capacity_limit = (size - writeable_size() + _capacity) * 2;
        std::unique_ptr<char[]> new_ptr(new char[new_capacity_limit]);
        memcpy(new_ptr.get(),_buffer.get(),_size);
        _buffer.reset();
        _buffer = std::move(new_ptr);
        _capacity = new_capacity_limit;
        append_data(data,size);
    }
}

size_t Buffer::read_all(char *dst){
    size_t ori_size = _size;
    if(read_data(dst,_size)){
        return ori_size;
    }
    return 0;
}

size_t Buffer::read_all(std::string& dst){
    size_t ori_size = _size;
    if(read_data(dst,_size)){
        return ori_size;
    }
    return 0;
}


bool Buffer::read_data(char *dst, size_t size){
    if(readable_size() < size){
        return false;
    }
    _size -= size;
    memcpy(dst, _buffer.get(), size);
    dst[size] = '\0';
    memcpy(_buffer.get(),_buffer.get()+size,_size);
    return true;
}

bool Buffer::read_data(std::string& dst,size_t size){
    if(readable_size() < size){
        return false;
    }
    _size -= size;
    dst += {_buffer.get(),size};
    memcpy(_buffer.get(), _buffer.get()+size, _size);
    return true;
}


bool Buffer::find_str(char* dst, char* str){
    char *pidx = empty()?nullptr:strstr(_buffer.get(),str);
    if(pidx == nullptr){
        return false;
    }
    return read_data(dst, pidx - _buffer.get() + strlen(str));
}

bool Buffer::find_str(std::string& dst,char *str){
    char *pidx =empty()?nullptr:strstr(_buffer.get(),str);
    if(pidx == nullptr){
        return false;
    }
    return read_data(dst, pidx - _buffer.get() + strlen(str));
}