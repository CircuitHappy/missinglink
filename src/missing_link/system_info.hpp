#include <cstdlib>

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
