#ifndef NETWORKINTERFACEFRONTENDBASE_H
#define NETWORKINTERFACEFRONTENDBASE_H

#include <systemc.h>

#include "inetworkinterfacefrontend.h"

/*!
 * \brief The NetworkInterfaceFrontEndBase class is a Base class that can be used both as an example of how to create a
 * front-end (shell module) to a IP-core or used directly as a Base class for a front-end.
 * It implements the methods from the interface of NetworkInterfaceFrontEnd and also has the obligatory events
 * necessaries for the communication between the front-end and back-end module.
 *
 * It has more easy to use methods for the communication and a message member which can be used for transmission of
 * messages.
 */
class NetworkInterfaceFrontEndBase : public INetworkInterfaceFrontEnd
{
    /*!
     * \brief A message member which can be used for send/receive messages to/from the back-end module.
     */
    std::vector<uint32_t> _payload;

    /*!
     * \brief A member used to hold a identification number when sending messages.
     */
    unsigned _payloadDst;

    /*!
     * \brief A member used to hold a identification number when receiving messages.
     */
    unsigned _payloadSrc;

    sc_event _ack, _valid;

    sc_event _writing, _reading;

protected:
    void sendPayload(const std::vector<uint32_t> &payload, int dst);

    void receivePayload(std::vector<uint32_t> &payload, int *src);
public:
    void kernelReceivePayload(std::vector<uint32_t> &payload, int *dst) override;

    void kernelSendPayload(const std::vector<uint32_t> &payload, int *src) override;
};

#endif // NETWORKINTERFACEFRONTENDBASE_H
