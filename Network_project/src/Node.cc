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


