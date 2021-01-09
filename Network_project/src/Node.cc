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
#include <cstdlib>

Define_Module(Node);

void Node::initialize()
{
    this->generateMsgs();
    // initialize
    this->max_seq = par("max_seq");
    this->S  = 0;
    this->Sf = 0;
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    this->R  = 0;
    this->ack = -1;

    EV << "Node constructed"<<endl;

}

void Node::handleMessage(cMessage *msg)
{
    //if self-message
    if (msg->isSelfMessage()) {
        switch(msg->getKind()) {

            // timeout: check if acknowledge came or not to re-send from the beginning of the window
            case 1:
            {
                bool compare = strcmp(msg->getName(), std::to_string(this->Sf).c_str());
                if(compare == 0) {
                    EV <<endl<< "Not receiving acknowledge  so go back from the beginning of the window !!" <<endl;
                    // timeout: no acknowledge came -> re-send
                    this->S = Sf;
                }
                break;
            }
            // send the next frame if there're remaining frames in the window
            case 2:
            {
                EV << endl << "Send a message normally" << endl;
                this->sendMsg();
                break;
            }
        }

    }

    // if out message arrived
    else {
        EV << "msg kind " << msg->getKind() <<endl;
        // frame + acknowledge
        if (msg->getKind() == 3){
            EV << "receiving frame and acknowledge" <<endl;

            this->post_receive_frame(msg);
            this->post_receive_ack(msg);

        }
        else if (msg->getKind() == 4) {
            this->sendMsg();
        }



    }

}

void Node::generateMsgs()
{
    // read the text file specified to node index
        std::stringstream ss;
        ss << getIndex();
        std::string file_name = "msg_files/" + ss.str() + ".txt";
        std::string msg;
        // read all lines into messages buffer
        std::ifstream file(file_name);
        while (getline (file, msg)) {
          this->msgs.push_back(msg);
        }
        //std::reverse(this->msgs.begin(), this->msgs.end());
        file.close();
}

void Node::sendMsg()
{
    EV << "S_f = "<<this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << " , R = " << this->R <<endl;

    // here may problem arise due to delayed messages
    if(this->S == int(this->msgs.size())) {
            // if current S reaches end of messages, terminate
            cMessage * end_session = new cMessage("end session");
            send(end_session ,"out");
            return;
     }

    // check if I reached the end of the window
    if (this->S <= this->Sl){
        // send message referred by S
        // TODO : perform hamming on message payload
        std::string framed_msg = this->addCharCount(this->msgs[this->S]);
        //EV << "msg payload = " << framed_msg << endl;
        const char* msg_payload = framed_msg.c_str();
        int padding = 0;
        msg_payload = this->computeHamming(msg_payload, padding).c_str();
        Imessage_Base * msg = new Imessage_Base(framed_msg.c_str());
        msg ->setSequence_number(this->S % (this->max_seq + 1));
        msg ->setMessage_payload(msg_payload);
        msg->setAcknowledge(this->ack);
        msg->setKind(3);
        msg->setPad_length(padding);
        send(msg,"out");

        // send self-message including sent_msg_seq_num to myself as a timer to re-send if S-f is still referring to this message
        cMessage * time_out_self_msg = new cMessage(std::to_string(this->S).c_str());
        time_out_self_msg->setKind(1);
        scheduleAt(simTime() + par("window_resend_timeout") , time_out_self_msg);

        EV << endl << "message sent successfully !" <<endl;
        EV << "S_f = "<<this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << " , R = " << this->R <<endl;

        this->S++;


    }

    // send self-message to send the next frame
    cMessage * send_next_self_msg = new cMessage("");
    send_next_self_msg->setKind(2);
    scheduleAt(simTime() + 1 , send_next_self_msg);

}

void Node::post_receive_ack(cMessage *msg){
    EV << endl <<"S_f = " << this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << ", message acknowledge = " << (((Imessage_Base *)msg)->getAcknowledge()) << endl;

    //initial frame sending
    if (((Imessage_Base *)msg)->getAcknowledge() == -1) {
        return;
    }
    while(this->Sf <= (((Imessage_Base *)msg)->getAcknowledge()))
    {
        this->Sf ++;
        //this->reset_R ++;
        //this->reset_ack ++;

    }
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    EV << " S_f = " << this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << endl;
}

void Node::post_receive_frame(cMessage *msg){
    // check in-order receive

    int received_frame_seq_num = ((Imessage_Base *)msg)->getSequence_number();
    if (received_frame_seq_num == this->R){
        //save the acknowledge to be sent
        this->ack = R;

        this->R++;

        const char * msg_payload = ((Imessage_Base *)msg)->getMessage_payload();
        std::string payload = this->decodeHamming(msg_payload, ((Imessage_Base *)msg)->getPad_length()).c_str();
        EV << " message payload after decode hamming " << payload << endl;
        bool check = this->checkCharCount(payload);
        if (check) {
            EV << "message char count right !!" <<endl;
        } else {
            EV << " message char count wrong !" << endl;
        }
        EV << " S_f = " << this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << " , R = " << this->R <<endl;
    }
}

// ------------------------------------- char count encode / decode ----------------------------------------------

std::string Node::addCharCount(std::string msg) {
    // add character count to message payload (framing)
    uint8_t msg_size = (uint8_t) msg.size();
    std::string framed_msg = "";
    framed_msg += msg_size;
    framed_msg += msg;
    return framed_msg;
}

bool Node::checkCharCount(std::string &msg) {
    // check whether character count is correct
    int msg_size = (int)msg[0];
    msg.erase(msg.begin());
    if (msg_size == msg.size()) {
        return true;
    }
    return false;
}

// ------------------------------------------- hamming encode / decode --------------------------------------------

std::string Node::computeHamming(std::string s, int &to_pad){
    int str_length = s.length();
    int m = 8 * str_length;
    int r = 1;
    while(m + r + 1 > pow(2 , r)){
        r++;
    }
    std::unordered_map<int, bool> parity_bits_exist;
    std::vector<int> parity_bits;
    for(int i = 0 ; i < r ; i++){
        parity_bits.push_back(pow(2,i));
        parity_bits_exist[pow(2,i)] = 1;
    }


    int total_bits = m + r;
    std::vector<bool> bits(total_bits+1);
    for(int i = 0 ; i < bits.size(); i++)
        bits[i] = 0;

    int curr_idx = 1;
    for(int i = 0 ; i < str_length; i++){
        std::bitset<8> char_bits(s[i]);
        for(int j = 7; j >= 0; j--){
            if(!parity_bits_exist[curr_idx]){
                bits[curr_idx++] = char_bits[j];
            }
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
        for(int j = curr_parity; j <= total_bits; j++){
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


    std::string result = "";

    if(total_bits % 8 != 0){
        to_pad = 8 - (total_bits % 8);
    }
    for(int i = 0 ; i < to_pad; i++)
        bits.push_back(0);

    for(int i = 1; i < bits.size(); i += 8){
        std::bitset<8> char_bits;
        for(int j = 0; j < 8 ; j++)
            char_bits[j] = bits[i+7-j];
        result += (char)char_bits.to_ulong();
    }
    return result;
}

std::string Node::decodeHamming(std::string s, int padding){
    int str_length = s.length();
    int mr = 8 * str_length - padding;
    int r = 1;
    while(mr + 1 > (pow(2,r))){
        r++;
    }
    std::unordered_map<int, bool> parity_bits_exist;
    std::vector<int> parity_bits;
    for(int i = 0 ; i < r ; i++){
        parity_bits.push_back(pow(2,i));
        parity_bits_exist[pow(2,i)] = 1;
    }

    int total_bits = mr;
    std::vector<bool> bits(total_bits + padding + 1);
    for(int i = 0 ; i < bits.size(); i++)
        bits[i] = 0;

    int curr_idx = 1;
    for(int i = 0 ; i < str_length; i++){
        std::bitset<8> char_bits(s[i]);
        for(int j = 7; j >= 0; j--){
            bits[curr_idx++] = char_bits[j];
        }
    }



    std::vector<bool> parity_vals(r);

    for(int i = 0 ; i < parity_bits.size() ; i++){
        int curr_parity = parity_bits[i];
        bool take = 1;
        int processed = 0;
        int count_ones = 0;
        for(int j = curr_parity; j <= total_bits; j++){
            if(take){
                if(j != curr_parity)
                    count_ones+=bits[j];
            }
            if(++processed == curr_parity){
                processed = 0;
                take = !take;
            }
        }
        parity_vals[i] = (bits[pow(2,i)] && (count_ones % 2 == 0)) || (!bits[pow(2,i)] && count_ones % 2 != 0);
    }


    int wrong_bit = 0;

    for(int i = 0 ; i < r; i++){
        wrong_bit += parity_vals[i] * (pow(2 , (i)));
    }

    bits[wrong_bit] = !bits[wrong_bit];

    std::vector<bool> msg_bits(mr - r);
    curr_idx = 0;

    for(int i = 1; i < bits.size(); i++){
        if(curr_idx == mr)
            break;
        if(!parity_bits_exist[i]){
            msg_bits[curr_idx++] = bits[i];
        }

    }


    std::string result = "";


    for(int i = 0; i < msg_bits.size(); i += 8){
        std::bitset<8> char_bits;
        for(int j = 0; j < 8 ; j++){
            char_bits[j] = msg_bits[i+7-j];
        }
        result += (char)char_bits.to_ulong();
    }
    return result;
}
