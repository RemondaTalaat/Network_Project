//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __NETWORK_PROJECT_NODE_H_
#define __NETWORK_PROJECT_NODE_H_

#include <omnetpp.h>
#include <bitset>
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

using namespace omnetpp;

class Node : public cSimpleModule
{
  private:
    std::vector<std::string> msgs {};    // hold the messages to be sent from the node

    int max_seq;                         // from node.ned - max sequence number to define the window size

    int S;                               // sequence number of the recently sent frame
    int Sf;                              // sequence number of the first frame in the window
    int Sl;                              // sequence number of the last frame in the window
    int R;                               // sequence number of the frame expected to be received
    int ack;                             // acknowledge of the last received frame

  protected:
    virtual void initialize();                                 // initialize the node data members and read its messages
    virtual void handleMessage(cMessage *msg);                 // handle any received message
    virtual void generateMsgs();                               // read the messages from out file to send them through the hub
    void sendMsg();                                            // send a message to the hub to navigate it to the selected receiver
    void post_receive_ack(cMessage *msg);                      // slide the window
    void post_receive_frame(cMessage *msg);                    // schedule ack
    std::string addCharCount(std::string msg);                 // add character count to message payload
    bool checkCharCount(std::string &msg);                     // check whether character count is correct
    std::string computeHamming(std::string s, int &to_pad);    // compute hamming code for message payload
    std::string decodeHamming(std::string s, int padding);     // decode hamming code and correct 1-bit error
};

#endif
