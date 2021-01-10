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

Define_Module(Node);
/**
 * initialize the node data members
 */
void Node::initialize()
{
    // read the messages from the file and save them in the node
    this->generateMsgs();
    // read the window size from ini file
    this->max_seq = par("max_seq");
    // initialize node as inactive and alive
    this->is_inactive = true;
    this->is_dead = false;
    // initialize the swapping window pointers
    this->S  = 0;
    this->Sf = 0;
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    // initialize the waited frame to 0
    this->R  = 0;
    // initialized the received acknowledge to -1 ( no thing received yet)
    this->ack = -1;
    EV << " node constructed !"<<endl;
}
/**
 * function is responsible for handling different types of received msgs
 * msg types:
       1- self-msg with type = 1 : resend from the beginning if its content not acked yet.
       2- self-msg with type = 2 : send next frame if window is not ended.
       3- out-msg  with type = 3 : msg carries ack + frame
       4- out-msg  with type = 4 : a session for this node started so start sending the messages
 * @param msg: the received message
 */
void Node::handleMessage(cMessage *msg)
{
    // check whether node is alive
    if (!this->is_dead)
    {
        //in case of self-message
        if (msg->isSelfMessage() && !this->is_inactive)
        {
            // check the type of the message
            switch(msg->getKind()) 
            {
                // timeout: check if acknowledge came or not to re-send from the beginning of the window
                case 1:
                {
                    bool compare = strcmp(msg->getName(), std::to_string(this->Sf).c_str());
                    if(compare == 0)
                    {
                        EV << endl << " not receiving acknowledge -> go back from the beginning of the window !" <<endl;
                        // timeout: no acknowledge came -> re-send
                        this->S = this->Sf;
                    }
                    break;
                }
                // send the next frame if there're remaining frames in the window
                case 2:
                {
                    this->sendMsg();
                    break;
                }
            }
        }
        // in case of out-message arrived
        else
        {
            // during session time receiving frame + ack
            if (msg->getKind() == 3 && !this->is_inactive)
            {
                this->post_receive_frame(msg);
                this->post_receive_ack(msg);
            }
            // start session message so start sending the messages
            else if (msg->getKind() == 4)
            {
                EV << " entering a new session" << endl;
                this->is_inactive = false;
                EV <<  " previous R value = " << this->R <<endl;
                this ->ack = -1;
                this -> R = std::stoi(msg->getName());
                EV << " new R value = " << this->R <<endl;
                this->sendMsg();
            }
            // end session message to change to inactive state
            else if (msg->getKind() == 5 && !this->is_inactive)
            {
                EV << " exiting current session" << endl;
                this->is_inactive = true;
                this->S = this->Sf;
                cMessage* msg = new cMessage(std::to_string(this->Sf).c_str());
                msg ->setKind(4);
                send(msg, "out");
                cancelAndDelete(this->send_next_self_msg);
            }
        }
    }
}
/**
 * read messages from each node's corresponding file and saves them in msgs vector data member
 */
void Node::generateMsgs()
{
    // read the text file specified to node index
    std::stringstream ss;
    ss << (getIndex()+1);
    std::string file_name = "msg_files/node" + ss.str() + ".txt";
    std::string msg;
    // read all lines into messages buffer
    std::ifstream file(file_name);
    while (getline (file, msg))
    {
        this->msgs.push_back(msg);
    }
    file.close();
}
/**
 * function used to send a message using go-back-n algorithm
 */
void Node::sendMsg()
{
    // send message referred by S if S stills in the window
    if (this->S <= this->Sl)
    {
        // first add char count framing to the message payload
        std::string framed_msg = this->addCharCount(this->msgs[this->S]);
        const char* msg_payload = framed_msg.c_str();
        int padding = 0;
        // apply hamming technique to the message payload for error detection and correction
        msg_payload = this->computeHamming(framed_msg, padding).c_str();
        // create the message to be sent
        Imessage_Base * msg = new Imessage_Base(framed_msg.c_str());
        // set the message parameters
        msg->setSequence_number(this->S);
        msg->setMessage_payload(msg_payload);
        msg->setAcknowledge(this->ack);
        msg->setKind(3);
        msg->setPad_length(padding);
        // send the message to the hub
        send(msg,"out");
        // send self-message including sent_msg_seq_num to myself as a timer to re-send if S-f is still referring to this message
        cMessage * time_out_self_msg = new cMessage(std::to_string(this->S).c_str());
        time_out_self_msg->setKind(1);
        scheduleAt(simTime() + par("window_resend_timeout") , time_out_self_msg);
        EV << endl << " message sent successfully !" <<endl;
        // print the window pointers status after sending the message
        EV << " S_f = "<<this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << " , R = " << this->R <<endl;
        this->S++;
    }
    // send self-message to send the next frame after 1 second
    this->send_next_self_msg = new cMessage("");
    send_next_self_msg->setKind(2);
    scheduleAt(simTime() + 1 , send_next_self_msg);
}
/**
 * slide the window when receiving acknowledge for the sent frame(s)
 * @param msg: the received message.
 */
void Node::post_receive_ack(cMessage *msg)
{
    //initial frame sending do nothing
    if (((Imessage_Base *)msg)->getAcknowledge() == -1)
    {
        return;
    }
    // slide the window based on the received acknowledge
    while(this->Sf <= (((Imessage_Base *)msg)->getAcknowledge()))
    {
        this->Sf ++;
    }
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    // if acknowledge is the last sequence number then the node is dead
    if (((Imessage_Base *)msg)->getAcknowledge() == int(this->msgs.size())-1)
    {
        EV << " node " << getIndex() << " is dead!" << endl;
        this->is_dead = true;
    }
    // print the new window pointers status( for debugging )
    EV << " S_f = " << this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << endl;
}
/**
 * receive a frame if it was expected, update the acknowledge to be sent and the next expected frame to come.
 * apply hamming decode to correct if error exists and extract the exact message
 * @param msg: the message received by the node
 */
void Node::post_receive_frame(cMessage *msg)
{
    // check in-order receive
    int received_frame_seq_num = ((Imessage_Base *)msg)->getSequence_number();
    //if the frame is the expected one take it else ignore it
    if (received_frame_seq_num == this->R)
    {
        //update the acknowledge value
        this->ack = R;
        // update the next expected frame
        this->R++;
        // get the message payload to extract the real message from it
        std::string msg_payload = ((Imessage_Base *)msg)->getMessage_payload();
        // apply hamming decode to get the exact message ( remove parity bits and correct any detected errors)
        std::string payload = this->decodeHamming(msg_payload, ((Imessage_Base *)msg)->getPad_length()).c_str();
        // print the real message framed by framing count
        EV << " message payload after decode hamming " << payload << endl;
        // check if error occured to the message count
        bool check = this->checkCharCount(payload);
        if (check)
        {
            EV << " message char count right !" <<endl;
        }
        else
        {
            EV << " message char count wrong !" << endl;
        }
        // print the window pointers status after receiving frame ( for debugging )
        EV << " S_f = " << this->Sf << " , S = " << this->S << " , S_l = " << this->Sl << " , R = " << this->R <<endl;
    }
}

// ------------------------------------- character count encode / decode ----------------------------------------------

/**
 * add character count to message payload (framing)
 * @param msg: the message payload
 */
std::string Node::addCharCount(std::string msg)
{
    // add character count to message payload (framing)
    uint8_t msg_size = (uint8_t) msg.size() + 1;
    std::string framed_msg = "";
    framed_msg += msg_size;
    framed_msg += msg;
    return framed_msg;
}
/**
 * check whether character count is correct
 * @param msg: the message payload
 */
bool Node::checkCharCount(std::string &msg)
{
    // check whether character count is correct
    if (!msg.size())
    {
        return false;
    }
    int msg_size = (int)msg[0] - 1;
    msg.erase(msg.begin());
    if (msg_size == msg.size())
    {
        return true;
    }
    return false;
}

// ------------------------------------------- hamming code encode / decode --------------------------------------------

/**
 * compute hamming code for message payload
 * @param msg: the message payload
 * @param to_pad: padding size
 */
std::string Node::computeHamming(std::string s, int &to_pad)
{
    int str_length = s.length();
    int m = 8 * str_length;
    int r = 1;
    while(m + r + 1 > pow(2 , r))
    {
        r++;
    }
    std::unordered_map<int, bool> parity_bits_exist;
    std::vector<int> parity_bits;
    for(int i = 0 ; i < r ; i++)
    {
        parity_bits.push_back(pow(2,i));
        parity_bits_exist[pow(2,i)] = 1;
    }
    int total_bits = m + r;
    std::vector<bool> bits(total_bits+1);
    for(int i = 0 ; i < bits.size(); i++)
        bits[i] = 0;
    int curr_idx = 1;
    for(int i = 0 ; i < str_length; i++)
    {
        std::bitset<8> char_bits(s[i]);
        for(int j = 7; j >= 0; j--)
        {
            if(!parity_bits_exist[curr_idx])
            {
                bits[curr_idx++] = char_bits[j];
            }
            else
            {
                curr_idx++;
                j++;
            }
        }
    }
    for(int i = 0 ; i < parity_bits.size() ; i++)
    {
        int curr_parity = parity_bits[i];
        bool take = 1;
        int processed = 0;
        int count_ones = 0;
        for(int j = curr_parity; j <= total_bits; j++)
        {
            if(take)
            {
                count_ones+=bits[j];
            }
            if(++processed == curr_parity)
            {
                processed = 0;
                take = !take;
            }
        }
        bits[curr_parity] = (count_ones % 2 == 0) ? 0 : 1;
    }
    std::string result = "";
    if(total_bits % 8 != 0)
    {
        to_pad = 8 - (total_bits % 8);
    }
    for(int i = 0 ; i < to_pad; i++)
        bits.push_back(0);
    for(int i = 1; i < bits.size(); i += 8)
    {
        std::bitset<8> char_bits;
        for(int j = 0; j < 8 ; j++)
            char_bits[j] = bits[i+7-j];
        result += (char)char_bits.to_ulong();
    }
    return result;
}
/**
 * decode hamming code and correct 1-bit error
 * @param msg: the message payload
 * @param padding: padding size
 */
std::string Node::decodeHamming(std::string s, int padding)
{
    if (!s.size())
    {
        return "";
    }
    int str_length = s.length();
    int mr = 8 * str_length - padding;
    int r = 1;
    while(mr + 1 > (pow(2,r)))
    {
        r++;
    }
    std::unordered_map<int, bool> parity_bits_exist;
    std::vector<int> parity_bits;
    for(int i = 0 ; i < r ; i++)
    {
        parity_bits.push_back(pow(2,i));
        parity_bits_exist[pow(2,i)] = 1;
    }
    int total_bits = mr;
    std::vector<bool> bits(total_bits + padding + 1);
    for(int i = 0 ; i < bits.size(); i++)
        bits[i] = 0;
    int curr_idx = 1;
    for(int i = 0 ; i < str_length; i++)
    {
        std::bitset<8> char_bits(s[i]);
        for(int j = 7; j >= 0; j--)
        {
            bits[curr_idx++] = char_bits[j];
        }
    }
    std::vector<bool> parity_vals(r);
    for(int i = 0 ; i < parity_bits.size() ; i++)
    {
        int curr_parity = parity_bits[i];
        bool take = 1;
        int processed = 0;
        int count_ones = 0;
        for(int j = curr_parity; j <= total_bits; j++)
        {
            if(take)
            {
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
    for(int i = 0 ; i < r; i++)
    {
        wrong_bit += parity_vals[i] * (pow(2 , (i)));
    }
    bits[wrong_bit] = !bits[wrong_bit];
    std::vector<bool> msg_bits(mr - r);
    curr_idx = 0;
    for(int i = 1; i < bits.size(); i++)
    {
        if(curr_idx == mr)
            break;
        if(!parity_bits_exist[i])
        {
            msg_bits[curr_idx++] = bits[i];
        }
    }
    std::string result = "";
    for(int i = 0; i < msg_bits.size(); i += 8)
    {
        std::bitset<8> char_bits;
        for(int j = 0; j < 8 ; j++)
        {
            char_bits[j] = msg_bits[i+7-j];
        }
        result += (char)char_bits.to_ulong();
    }
    return result;
}
