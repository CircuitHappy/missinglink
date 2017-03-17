
#pragma once

namespace MissingLink {
namespace GPIO {

typedef enum {
    LOW = 0,
    HIGH = 1
} Value;
    
class Output {
    virtual void write(Value value) = 0;
};
    
class Input {
    virtual Value read() const = 0;
};

}}
