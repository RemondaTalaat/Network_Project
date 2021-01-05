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
#include <bitset>
#include <vector>
#include <unordered_map>

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

string Node::computeHamming(string s, int &to_pad){
    int str_length = s.length();
    int m = 8 * str_length;
    int r = 1;
    while(m + r + 1 > (2 ^ r)){
        r++;
    }
    unordered_map<bool, bool> parity_bits_exist;
    vector<bool> parity_bits;
    for(int i = 0 ; i < r ; i++){
        parity_bits.push_back(2^i);
        parity_bits_exist[2^i] = 1;
    }

    int total_bits = m + r;
    vector<bool> bits(total_bits+1);
    for(int i = 0 ; i <= total_bits; i++)
        bits[i] = 0;

    int curr_idx = 0;
    for(int i = 0 ; i < str_length; i++){
        bitset<8> char_bits(s[i]);
        for(int j = 7; j >= 0; j--){
            curr_idx++;
            if(!parity_bits_exist[curr_idx])
                bits[curr_idx++] = char_bits[j];
            else
                j++;
        }
    }

    for(int i = 0 ; i < parity_bits.size() ; i++){
        int curr_parity = parity_bits[i];
        bool take = 1;
        int processed = 0;
        int count_ones = 0;
        for(int j = 1; j <= total_bits; j++){
            if(take){
                count_ones+=bits[j];
            }
            if(++processed == curr_parity){
                processed = 0;
                take = !take;
            }
        }
        bits[curr_parity] = (count_ones % 2 == 0) ? 0 : 1;
    }

    string result = "";

    if(total_bits % 8 != 0){
        to_pad = 8 - (total_bits % 8);
    }
    for(int i = 0 ; i < to_pad; i++)
        bits.push_back(0);

    for(int i = 1; i < bits.size(); i += 8){
        bitset<8> char_bits;
        for(int j = 0; j < 8 ; j++)
            char_bits[j] = bits[i+7-j];
        result += (char)char_bits.to_ulong();
    }
    return result;

}

void Node::cirInc()
{
    this->S++;
    // if (this->S > this->Sl){
    //     this->S = 0;
    // }
}


