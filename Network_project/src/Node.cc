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
#include <cstdlib>

Define_Module(Node);

void Node::initialize()
{
    this->generateMsgs();
    // initialize
    this->max_seq = par("max_seq");
    this->S = 0; 
    this->Sf = 0;
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    this->R = 0;
    this->ack_buffer = 0;

    EV << "Node constructed";

    this->sendMsg();

    
}

void Node::handleMessage(cMessage *msg)
{
    //if self-msg
    if (msg->isSelfMessage()) {
        switch(msg->getKind()) {

            // timeout: check if ack came or not to resend from the beginning of the window
            case 1:
            {
                bool compare = strcmp(msg->getName(), std::to_string(this->Sf).c_str());
                if(compare == 0) {
                    // timeout: no ack came -> resend
                    this->S = Sf;
                }
                break;
            }
            
            // send the next frame if there're remaining frames in the window
            case 2:
            {
                this->sendMsg();
                break;
            }

            // receeive timeout self-msgs to send ack without piggy backing if not sent yet
            case 3:
            {
                // check if the received frame to be acknowledged is the buffered frame to be acknowledged (i.e. not sent yet)
                bool compare = strcmp(msg->getName(), std::to_string(this->acks_to_be_sent.front()).c_str());
                // EV << endl << msg->getName() << endl << std::to_string(this->ack_buffer).c_str() << endl ;
                // EV << endl << 222222222222222 << endl;
                if(compare == 0) {
                    // ack is not sent -> send it without piggy backing
                    // send seq number of the frame to be acknowledged
                    int ack = this->acks_to_be_sent.front();
                    const char* msg_payload = std::to_string(ack).c_str();
                    Imessage_Base * msg = new Imessage_Base(msg_payload);
                    msg->setMessage_payload(msg_payload);
                    msg->setAcknowledge(ack);
                    msg->setKind(1);
                    send(msg,"out"); 
                        
                    EV << endl << msg->getName() << endl << std::to_string(ack).c_str() << endl ;
                    EV << endl << 8888888888888 << endl;
                    // TODO: post_send_ack - circular inc
                    // this->ack_buffer ++;
                    // this->ack_buffer = this->ack_buffer % (this->max_seq + 1);
                    this->acks_to_be_sent.pop();
                    
                       
                } 
            }

        }

    }

    // if out msg arrived
    else {

        // frame only
        if (msg->getKind() == 2){

            this->post_receive_frame(msg);
            
        }

        // ack only
        else if (msg->getKind() == 1){

            this->post_receive_ack(msg);

        }

        // frame + ack 
        else if (msg->getKind() == 3){

            this->post_receive_ack(msg);
            this->post_receive_frame(msg);

        }
         


    }

}

void Node::generateMsgs()
{
    // TODO: read from random file
    this->msgs.push_back("hi there baby!");
    this->msgs.push_back("hello!");
    this->msgs.push_back("how are you?");
    this->msgs.push_back("I love you");
    this->msgs.push_back("hi1");
    this->msgs.push_back("hi2");
    this->msgs.push_back("hi3");
    this->msgs.push_back("hi4");
    this->msgs.push_back("hi5");

}

void Node::sendMsg()
{

    // check if I reached the end of the window
    if (this->S <= this->Sl){
        // send msg referred by S
        const char* msg_payload = this->msgs[S].c_str();
        Imessage_Base * msg = new Imessage_Base(msg_payload);
        msg ->setSequence_number(this->S % (this->max_seq + 1));
        msg ->setMessage_payload(msg_payload);
        // piggy backing frame + ack
        if (!this->acks_to_be_sent.empty()){
            int ack = this->acks_to_be_sent.front();
            msg->setAcknowledge(ack);
            msg->setKind(3);
            this->acks_to_be_sent.pop();
        }
        // frame only
        else{
            msg->setKind(2);
        }
        
        
        send(msg,"out");

        // send self-msg including sent_msg_seq_num to myself as a timer to re-send if Sf is still referring to this msg
        cMessage * time_out_self_msg = new cMessage(std::to_string(this->S).c_str());
        time_out_self_msg->setKind(1);
        scheduleAt(simTime() + par("window_resend_timeout") , time_out_self_msg);

        EV << 555555555555555;
        EV << endl << this->Sf << endl << this->S << endl << this->Sl << endl;

        // increment S "circular increment"
        this->S++;

        
    }
    
    // send self-msg to send the next frame
    cMessage * send_next_self_msg = new cMessage("");
    send_next_self_msg->setKind(2);
    scheduleAt(simTime() + 1 , send_next_self_msg);

}

string Node::computeHamming(string s, int &to_pad){
    int str_length = s.length();
    int m = 8 * str_length;
    int r = 1;
    while(m + r + 1 > (2 ^ r)){
        r++;
    }
    unordered_map<int, bool> parity_bits_exist;
    vector<bool> parity_bits;
    for(int i = 0 ; i < r ; i++){
        parity_bits.push_back(2^i);
        parity_bits_exist[2^i] = 1;
    }

    int total_bits = m + r;
    vector<bool> bits(total_bits+1);
    for(int i = 0 ; i < bits.size(); i++)
        bits[i] = 0;

    int curr_idx = 0;
    for(int i = 0 ; i < str_length; i++){
        bitset<8> char_bits(s[i]);
        for(int j = 7; j >= 0; j--){
            if(!parity_bits_exist[curr_idx])
                bits[curr_idx++] = char_bits[j];
            else{
                curr_idx++;
                j++;
            }
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

string Node::decodeHamming(string s, int padding){
    int str_length = s.length();
    int mr = 8 * str_length - padding;
    int r = 1;
    while(mr + 1 > (2 ^ r)){
        r++;
    }
    unordered_map<bool, bool> parity_bits_exist;
    vector<bool> parity_bits;
    for(int i = 0 ; i < r ; i++){
        parity_bits.push_back(2^i);
        parity_bits_exist[2^i] = 1;
    }

    int total_bits = mr;
    vector<bool> bits(total_bits + padding + 1);
    for(int i = 0 ; i < bits.size(); i++)
        bits[i] = 0;

    int curr_idx = 0;
    for(int i = 0 ; i < str_length; i++){
        bitset<8> char_bits(s[i]);
        for(int j = 7; j >= 0; j--){
            bits[curr_idx++] = char_bits[j];
        }
    }

    vector<bool> parity_vals(r);

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
        parity_vals[i] = (bits[2^i] && (count_ones % 2 == 0)) || (!bits[2 ^ i] && count_ones % 2 != 0);
    }

    int wrong_bit = 0;

    for(int i = r-1 ; i >= 0; i--){
        int idx = r - i - 1;
        wrong_bit += parity_vals[i] * (2 ^ (idx));
    }

    bits[wrong_bit] = !bits[wrong_bit];

    vector<bool> msg_bits(mr - r);
    curr_idx = 0;

    for(int i = 1; i <= bits.size(); i++){
        if(curr_idx == mr)
            break;
        if(!parity_bits_exist[i]){
            msg_bits[curr_idx++] = bits[i];
        }
        else
            curr_idx++;
    }

    string result = "";


    for(int i = 0; i < msg_bits.size(); i += 8){
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
void Node::post_receive_ack(cMessage *msg){
    EV << this->Sf << endl << this->S << endl << this->Sl << endl << (((Imessage_Base *)msg)->getAcknowledge()) << endl;
    // this->Sf += abs((((Imessage_Base *)msg)->getAcknowledge()) - this->Sf % (this->max_seq + 1)) + 1;
    
    // slide the window
    while(this->Sf % (this->max_seq + 1) != (((Imessage_Base *)msg)->getAcknowledge())){
        this->Sf ++;
    }
    this->Sf ++;
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    EV << endl << 9999999999 << endl;
    EV << endl << this->Sf << endl << this->S << endl << this->Sl << endl;
}

void Node::post_receive_frame(cMessage *msg){
    // check inorder receive
    int received_frame_seq_num = ((Imessage_Base *)msg)->getSequence_number();
    if (received_frame_seq_num == this->R){
        // add the ack to the buffer
        this->acks_to_be_sent.push(received_frame_seq_num);
        
        // add a self-msg as a timeout that after it the ack will be sent alone -> no piggy backing
        cMessage * ack_time_out_self_msg = new cMessage(std::to_string(received_frame_seq_num).c_str());
        ack_time_out_self_msg->setKind(3);
        scheduleAt(simTime() + par("ack_send_timeout") , ack_time_out_self_msg);  

        // increase next frame expected to be received (circular increment)  
        this->R++;
        this->R = this->R % (this->max_seq + 1);
    }
}


