#include <cstdlib>
#include <rtmidi/RtMidi.h>

namespace MissingLink {

class SysInfo {

  public:

    SysInfo();
    virtual ~SysInfo();

    std::string GetIP();

  protected:

    void open();
    void close();

};

}//namespaces
