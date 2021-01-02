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

#include "Node.h"
#include "Imessage_m.h"
#include <string>

Define_Module(Node);

void Node::initialize()
{
    this->generateMsgs();
    // initialize
    this->max_seq = par("max_seq");
    this->S = 0; // initialize with max so that circular increment gets it back to 0
    this->Sf = 0;
    this->Sl = max_seq;

    EV << "Node constructed";

    this->sendMsg();
    // Imessage_Base * msg = new Imessage_Base("Dummy Message");
    // // Dummy message header
    // msg ->setSequence_number(1);
    // msg ->setPad_length(4);
    // msg ->setAcknowledge(2);
    // // Dummy message pay load
    // msg ->setMessage_payload("pay load");

    // //sending the dummy message
    // send(msg,"out");
    
}

void Node::handleMessage(cMessage *msg)
{
    //if self-msg -> rewind

    if (msg->isSelfMessage()) {
        // cstring msg_payload =std::to_string(this->Sf).c_str();
        bool compare = strcmp(msg->getName(), std::to_string(this->Sf).c_str());
        if(compare == 0) {
            this->S = Sf;
        }
    }


    //if ack message -> inc Sl and Sf

}

void Node::generateMsgs()
{
    // TODO: read from random file
    this->msgs.push_back("hi there!");
    this->msgs.push_back("hello!");
    this->msgs.push_back("how are you?");

}

void Node::sendMsg()
{
    

    // send msg referred by S
    // cstring msg_payload = this->msgs[S].c_str();
    const char* msg_payload = this->msgs[S].c_str();
    Imessage_Base * msg = new Imessage_Base(msg_payload);
    msg ->setSequence_number(this->S % (this->max_seq + 1));
    msg ->setMessage_payload(msg_payload);
    send(msg,"out");

    // increment S "circular increment"
    this->S++;

    // send self-msg including sent_msg_seq_num to myself as a timer to re-send if Sf is still referring to this msg
    // msg_payload = std::to_string(this->S).c_str();
    // scheduleAt(simTime() + 120 , new cMessage(std::to_string(this->S).c_str()));

}

void Node::cirInc()
{
    this->S++;
    // if (this->S > this->Sl){
    //     this->S = 0;
    // }
}


