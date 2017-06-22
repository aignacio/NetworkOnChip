#include "networkinterface.h"

NetworkInterface::NetworkInterface(sc_module_name name, unsigned id) :
    sc_module(name),
    _networkInterfaceId(id),
    _frontEnd(nullptr)
{
    SC_THREAD(_threadRead);
    SC_THREAD(_threadWrite);
}

void NetworkInterface::connectFrontEnd(INetworkInterfaceFrontEnd *networkInterfaceFrontEnd)
{
    _frontEnd = networkInterfaceFrontEnd;
}

void NetworkInterface::_threadRead()
{
    if (!_frontEnd) {
        std::string message("Front-End Not Connected NI-Id:");
        NoCDebug::printDebug(message + std::to_string(_networkInterfaceId) + " Name: " + name(), NoCDebug::NI, true);
    }
    for (;;) {
        // Receive Message from Front-End
        wait(_frontEnd->sendFrontEndValidEvent());
        std::vector<uint32_t> receivedMessage;
        _frontEnd->sendMessage(&receivedMessage);
        unsigned destinationId;
        _frontEnd->sendMessageDestination(&destinationId);
        _frontEnd->receiveBackEndAcknowledgeEvent();

        // Packet Message
        _packMessage(destinationId, receivedMessage);
        receivedMessage.clear();

        // Send to Router
        _sendToRouter();
    }
}

void NetworkInterface::_threadWrite()
{
    for (;;) {
        // Receive from router
        _receiveFromRouter();

        // Unpack message
        unsigned sourceId;
        std::vector<uint32_t> sendMessage;
        _unpackMessage(&sourceId, &sendMessage);

        // Send Message to front-End
        _frontEnd->receiveMessage(&sendMessage);
        _frontEnd->receiveMessageSource(&sourceId);
        _frontEnd->receiveBackEndValidEvent();
        wait(_frontEnd->sendFrontEndAcknowledgeEvent());
    }
}

void NetworkInterface::_packMessage(unsigned destinationId, const std::vector<uint32_t> &message)
{
    NoCDebug::printDebug("NI-ID:" + std::to_string(_networkInterfaceId) + " packeting message.", NoCDebug::NI);
    _sendPacket.clear();
    uint16_t packetSize = static_cast<uint16_t>(std::min(static_cast<size_t>(std::numeric_limits<uint16_t>::max()),
                                                         message.size()));
    // Create Head Flit
    flit_t headerData = 0;

    headerData.range(15, 0) = packetSize;
    headerData.range(23, 16) = destinationId;
    headerData.range(31, 24) = _networkInterfaceId;
    Flit *flit = new Flit(headerData, 0);
    _sendPacket.push_back(flit);

    // Create Tail Flits
    for (uint16_t flitIndex = 0; flitIndex < packetSize; flitIndex++) {
        flit_t tailFlit = message.at(flitIndex);
        flit = new Flit(tailFlit, 0);
        _sendPacket.push_back(flit);
    }

    // Note: It's reponsability of the NetworkInterface receiver of collecting and delete all the references of
    // the Flit objects.
}

const void NetworkInterface::_unpackMessage(unsigned *sourceId, std::vector<uint32_t> *message)
{
    NoCDebug::printDebug("NI-ID:" + std::to_string(_networkInterfaceId) + " unpacketing message.", NoCDebug::NI);
    // Unpack Head Flit
    Flit *flit = _receivePacket.at(0);
    *sourceId = flit->getData().range(31, 24);
    message->clear();
    uint16_t packetSize = flit->getData().range(23, 16);
    for (uint16_t flitIndex = 1; flitIndex <= packetSize; flitIndex++) {
        flit = _receivePacket.at(flitIndex);
        message->push_back(flit->getData().to_uint());
    }

    // Now delete all the flits.
    for (Flit *f : _receivePacket) {
        if (f) {
            delete f;
        }
    }
    _receivePacket.clear();
}

void NetworkInterface::_sendToRouter()
{
    for (Flit *flit : _sendPacket) {
        localChannel->validSender();
        localChannel->sendFlit(flit);
        wait(*localChannel->acknowledgeSender());
    }
}

void NetworkInterface::_receiveFromRouter()
{
    _receivePacket.clear();
    Flit *flit;
    // Receive Header Flit
    localChannel->validReceiver();
    localChannel->receiveFlit(flit);
    wait(*localChannel->acknowledgeReceiver());
    _receivePacket.push_back(flit);

    // Receive Tail Flits
    uint16_t packetSize = flit->getData().range(15, 0);
    for (uint16_t flitIndex = 0; flitIndex < packetSize; flitIndex++) {
        localChannel->validReceiver();
        localChannel->receiveFlit(flit);
        wait(*localChannel->acknowledgeReceiver());
        _receivePacket.push_back(flit);
    }
}

