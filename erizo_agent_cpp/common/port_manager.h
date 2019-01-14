#ifndef PORT_MANAGER_H
#define PORT_MANAGER_H

#include <logger.h>

class PortManager
{
    DECLARE_LOGGER();

  public:
    ~PortManager();
    static PortManager *getInstance();

    int allocPort(unsigned short &port);

  private:
    int checkPort(unsigned short port);
    PortManager();

  private:
    static PortManager *instance_;
};

#endif